/*
* Copyright (c) 2002 Nokia Corporation and/or its subsidiary(-ies). 
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of the License "Symbian Foundation License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.symbianfoundation.org/legal/sfl-v10.html".
*
* Initial Contributors:
* Nokia Corporation - initial contribution.
*
* Contributors:
*
* Description:  Implementation for movie processor
*
*/


//  Include Files

#include "movieprocessorimpl.h"
#include "statusmonitor.h"
#include "activequeue.h"
#include "mp4parser.h"
#include "mp4composer.h"
#include "videoencoder.h"
#include "yuv2rgb12.h"
#include "yuv2rgb16.h"
#include "yuv2rgb24.h"
#include "VideoProcessorAudioData.h"
#include "DisplayChain.h"
#include "VedRgb2YuvConverter.h"
#include "vedaudiosettings.h"
#include "vedvideosettings.h"
#include "AudSong.h"
#include "audioprocessor.h"
#include "SizeEstimate.h"
#include "vedavcedit.h"

//  Local Constants

const TUint KReadBufInitSize = 512; // stream start buffer initial size
//const TInt	KVideoProcessorPriority = CActive::EPriorityHigh;
const TUint KVideoQueueBlocks = 16;
const TUint KVideoQueueBlockSize = 256;
const TInt	KDemuxPriority = CActive::EPriorityHigh;
const TUint	KInitialAudioBufferSize = 1024;
const TUint KInitialVideoBufferSize = 10240;
const TUint KMaxVideoSpeed = 1000;
const TUint KVideoTimeScale = 5000; // for both normal & generated clips
const TUint KAMRAudioTimeScale = 8000;

const TUint KDiskSafetyLimit = 400000;   // Amount of free disk space to leave unused

_LIT(KTempFilePath ,"c:\\system\\temp\\");  // path for temp file used in image insertion

// An assertion macro wrapper to clean up the code a bit
#define VPASSERT(x) __ASSERT_DEBUG(x, User::Panic(_L("CMovieProcessorImpl"), EInvalidInternalState))

#ifdef _DEBUG
#include <e32svr.h>
#define PRINT(x) RDebug::Print x;
#else
#define PRINT(x)
#endif

// ================= MEMBER FUNCTIONS =======================

// -----------------------------------------------------------------------------
// CMovieProcessorImpl::NewL
// Two-phased constructor.
// -----------------------------------------------------------------------------
//
CMovieProcessorImpl* CMovieProcessorImpl::NewL() 
{
	CMovieProcessorImpl* self = NewLC();
	CleanupStack::Pop(self);
	return self;
}

CMovieProcessorImpl* CMovieProcessorImpl::NewLC()
{
	CMovieProcessorImpl* self = new (ELeave) CMovieProcessorImpl();
	CleanupStack::PushL(self);
	self->ConstructL();
	return self;
}

// -----------------------------------------------------------------------------
// CMovieProcessorImpl::CMovieProcessorImpl
// C++ default constructor can NOT contain any code, that
// might leave.
// -----------------------------------------------------------------------------
//
CMovieProcessorImpl::CMovieProcessorImpl()
: CActive(EPriorityNormal), iReadImageDes(0,0), iReadDes(0, 0)
{
  // Reset state
    iState = EStateIdle;
	iDataFormat = EDataAutoDetect;
    iAudioFramesInSample = KVedAudioFramesInSample;

    iStartTransitionEffect = EVedStartTransitionEffectNone; 
	iMiddleTransitionEffect = EVedMiddleTransitionEffectNone; 
	iPreviousMiddleTransitionEffect = EVedMiddleTransitionEffectNone;
	iEndTransitionEffect = EVedEndTransitionEffectNone; 
	iSpeed = KMaxVideoSpeed; 
	iColorEffect = EVedColorEffectNone;
    iNumberOfVideoClips=1; 	
    iTr.iTrPrevNew = -1;
	iTr.iTrPrevOrig = -1;	
    iStartFrameIndex = -1;
    iMovieSizeLimit = 0;
    iFrameParametersSize = 0;
    
    // We are now properly initialized
    iState = EStateIdle;
	
}

// -----------------------------------------------------------------------------
// CMovieProcessorImpl::~CMovieProcessorImpl
// Destructor.
// -----------------------------------------------------------------------------
//
CMovieProcessorImpl::~CMovieProcessorImpl()
{

    Cancel();

    TInt error = KErrNone;

	TRAP(error, DoCloseVideoL());	

    DeleteClipStructures();       

	if (iFrameParameters)
    {
		User::Free(iFrameParameters);			
        iFrameParameters = 0; 
    }
        
	if (iVideoClipParameters)
    {
		User::Free(iVideoClipParameters);	
        iVideoClipParameters = 0; 
    }

    if (iOutAudioBuffer) {
		delete iOutAudioBuffer; 
        iOutAudioBuffer=0;    
    }

    if (iOutVideoBuffer) {
		delete iOutVideoBuffer; 
        iOutVideoBuffer=0;    
    }

	// although this should be released by VideoEditorEngine, 
	// the following is still needed in case of leave 

	if(iDemux)
    {
		delete iDemux;				
        iDemux = 0; 
    }

	if(iVideoProcessor)
    {
		delete iVideoProcessor;	
        iVideoProcessor = 0;	
    }

    if (iComposer) {
        delete iComposer; 
        iComposer = 0;
    }
    
    if (iAudioProcessor)
    {
        delete iAudioProcessor;
        iAudioProcessor = 0;	
    }        

	if(iImageComposer) 
	{
		delete iImageComposer;						
		iImageComposer=0;
	}
	
	if (iImageAvcEdit)
	{
	    delete iImageAvcEdit;
	    iImageAvcEdit = 0;
	}

	if (iYuvImageBuf)
	{
		User::Free(iYuvImageBuf);
		iYuvImageBuf=0;
	}

    if(iVideoEncoder) 
    {
		delete iVideoEncoder;	
        iVideoEncoder = 0; 
    }
	
	if(iParser)
    {
        delete iParser;
        iParser = 0;
    }
	if(iVideoQueue)
    {
		delete iVideoQueue;		
        iVideoQueue = 0;
    }

    if (iWaitScheduler)
    {
        delete iWaitScheduler;
        iWaitScheduler = 0;
    }

	if(iMonitor)
    {
		delete iMonitor;			
        iMonitor = 0; 	
    }
    
    if (iReadBuf)
		User::Free(iReadBuf);

    if (iRgbBuf)
    {
        delete iRgbBuf;
        iRgbBuf = 0;
    }        
    
    if (iOutBitmap)
    {
        delete iOutBitmap;
        iOutBitmap = 0;
    }
    
    if (iSizeEstimate)
    {
        delete iSizeEstimate;
        iSizeEstimate = 0;
    }
    
    if (iAvcEdit)
    {
        delete iAvcEdit;
        iAvcEdit = 0;
    }
    
    if (iImageYuvConverter)
    {
        delete iImageYuvConverter;
        iImageYuvConverter = 0;
    }

	// for transition effect
    if ( iFsConnected )
    {
        TRAP(error, CloseTransitionInfoL());
        iFs.Close();
        iFsConnected = EFalse;
    }
}


// -----------------------------------------------------------------------------
// CMovieProcessorImpl::ConstructL
// Symbian 2nd phase constructor can leave.
// -----------------------------------------------------------------------------
//
void CMovieProcessorImpl::ConstructL()
{	
	
	// Allocate stream reading buffer
	iReadBuf = (TUint8*) User::AllocL(KReadBufInitSize);
	iBufLength = KReadBufInitSize;
	
	iClipFileName.Zero();
	iOutputMovieFileName.Zero();    

    iWaitScheduler = new (ELeave) CActiveSchedulerWait;

    // Add to active scheduler
    CActiveScheduler::Add(this);
    
    iSizeEstimate = CSizeEstimate::NewL(this);

    iState = EStateIdle;
}


// -----------------------------------------------------------------------------
// CMovieProcessorImpl::StartMovieL
// Prepares the processor for processing a movie and starts processing  
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CMovieProcessorImpl::StartMovieL(CVedMovieImp* aMovie, const TDesC& aFileName, RFile* aFileHandle,
                                      MVedMovieProcessingObserver* aObserver)
{

    PRINT((_L("CMovieProcessorImpl::StartMovieL() begin")))

    // reset member variables
    ResetL();

	// get arguments 
	iMovie = aMovie; 
    if (!iMovie)
        User::Leave(KErrArgument);

	iOutputMovieFileName = aFileName;
	iOutputFileHandle = aFileHandle;	
    
    CVedMovieImp* movie = (iMovie);
    iObserver = aObserver;

    if (!iObserver)
        User::Leave(KErrArgument);    

    if (iMonitor) 
    {
        delete iMonitor;
        iMonitor = 0;
    }    
    
     // Create a status monitor object:
	iMonitor = new (ELeave) CStatusMonitor(iObserver, this, aMovie);
	iMonitor->ConstructL();
	
	// update movie properties	
	iFramesProcessed=0;
	iNumberOfVideoClips = iMovie->VideoClipCount();
    iNumberOfAudioClips = iMovie->AudioClipCount();    

	// calculate total movie duration for progress bar: video & audio tracks.
	// in milliseconds
	iTotalMovieDuration = TInt64(2) * ( movie->Duration().Int64()/1000 );
    
    for (TInt i = 0; i < iNumberOfVideoClips; i++)
    {
        CVedVideoClip* currentClip = movie->VideoClip(i);
		CVedVideoClipInfo* currentInfo = currentClip->Info();

        // Take time to generate a clip into account
        if (currentInfo->Class() == EVedVideoClipClassGenerated)
        {            
            iTotalMovieDuration += currentInfo->Duration().Int64()/1000;
        }
    }    

	// set media types
	SetOutputMediaTypesL();

	// get transcode factors: bitstream mode & time inc. resolution
	GetTranscodeFactorsL();

	// set video transcoding parameters
	SetupTranscodingL();
	
#ifdef VIDEOEDITORENGINE_AVC_EDITING
	// check if AVC editing is involved
	TBool avcEditing = ( iOutputVideoType == EVideoAVCProfileBaseline );		
    for(TInt i = 0; i < movie->VideoClipCount() && avcEditing == EFalse; i++)
	{
		CVedVideoClip* currentClip = movie->VideoClip(i);
		CVedVideoClipInfo* currentInfo = currentClip->Info();
				
		if( currentInfo->Class() == EVedVideoClipClassFile &&
		   (currentInfo->VideoType() == EVedVideoTypeAVCBaselineProfile) )
		{
		    avcEditing = ETrue;
		    break;
	    }
	}		
	
	if (avcEditing)
	{
	    // create AVC editing instance
	    iAvcEdit = CVedAVCEdit::NewL();	    
	}
	
	if (iOutputVideoType == EVideoAVCProfileBaseline)
	{
	    // set level
	    iAvcEdit->SetOutputLevel( GetOutputAVCLevel() );
	}
		
#endif

	// Check
	//iOutputAudioType = EAudioAMR;						// default is Amr if all are generated

	if ( iFsConnected == EFalse )
	{
        User::LeaveIfError( iFs.Connect() );
        iFsConnected = ETrue;
	}

    if (iNumberOfVideoClips)
    {
        iStartTransitionEffect = aMovie->StartTransitionEffect();
        iEndTransitionEffect = aMovie->EndTransitionEffect();	
		CloseTransitionInfoL();
    }

	iImageEncodeProcFinished = 0; // to indicate whether encode images process is finished
	iImageEncodedFlag = 0;  // has an image been encoded
	iFirstTimeProcessing = 0;
	iTotalImagesProcessed = 0;  
	iImageClipCreated = 0;  // has an image clip been created
	
	iImageVideoTimeScale = KVideoTimeScale; // Initializing to standard value
	iGetFrameInProgress = 0;
	iOutputAudioTimeSet = 0;
	iOutputVideoTimeSet = 0;
	iAllGeneratedClips = 0;   // Indicates if all the clips in the movie are generated clips - 0 indicates false
	iEncodeInProgress = 0;     // Indicates encoding is not in progress		
	iFirstClipHasNoDecInfo = EFalse;
	
	// Allocate memory for frame parameters array
	if (iFrameParameters)
        User::Free(iFrameParameters);
    
    iFrameParameters = 0;
	iFrameParameters = (struct TFrameParameters *)User::AllocL(iFrameParametersSize * sizeof(struct TFrameParameters));
	Mem::Fill(iFrameParameters, iFrameParametersSize * sizeof(TFrameParameters), 0);
	
	InitializeClipStructuresL();
	
	// second pass starts here 
	// load first clip properties 
	iVideoClipNumber=0; 
    iAudioClipNumber=0;	   

    if (iNumberOfVideoClips)
        iVideoClip = movie->VideoClip(iVideoClipNumber);    

	// if clip is not file-based, then call some other initializer
	TInt firstClipIsGenerated = 0;
	iFirstClipIsGen = EFalse;

	// what if iNumberOfVideoClips == 0, this will fail !
	if(iNumberOfVideoClips && iVideoClip->Info()->Class() == EVedVideoClipClassGenerated)
	{
		// since frame parameters are not available here, use temporary instantiation for composer
		TemporaryInitializeGeneratedClipL(); 
		// note that we may need to create a parser temporarily
		firstClipIsGenerated = 1;
		iFirstClipIsGen = ETrue;
	}
	else
	{
		InitializeClipL();	// check details inside initialization
	}

	if(iOutputAudioType == EAudioAMR)
	{
		if( iAudioType == EAudioNone)
			iAudioType = EAudioAMR;
		iAudioFramesInSample = KVedAudioFramesInSample;
	}

	else if(iOutputAudioType == EAudioAAC)
	{
		if( iAudioType == EAudioNone)
		{
			iAudioType = EAudioAAC;	
			iFirstClipHasNoDecInfo = ETrue; // because it has no audio so it will have no decoder specific Info			
		}
		iAudioFramesInSample = 1;
	}


	VPASSERT(!iComposer);
    // initialize composer
    if (iOutputFileHandle)
        iComposer = CMP4Composer::NewL(iOutputFileHandle, (CParser::TVideoFormat)iOutputVideoType, (CParser::TAudioFormat)iOutputAudioType, iAvcEdit);
    else
	    iComposer = CMP4Composer::NewL(iOutputMovieFileName, (CParser::TVideoFormat)iOutputVideoType, (CParser::TAudioFormat)iOutputAudioType, iAvcEdit);

    iFramesProcessed = 0;
    iStartingProcessing = ETrue;
    
    VPASSERT(iOutputVideoTimeScale);
    VPASSERT(iOutputAudioTimeScale);
    VPASSERT(iAudioFramesInSample);    

    // write video & audio descriptions
    CComposer::TStreamParameters streamParameters;

	if(iAllGeneratedClips == 0)  // if all were generated initialize to default
	{
		if (iNumberOfVideoClips) 
		{
			streamParameters = (CComposer::TStreamParameters &)iParser->iStreamParameters;
			TSize tmpSize = iMovie->Resolution();
			streamParameters.iVideoWidth = tmpSize.iWidth;    /* iVideoParameters.iWidth;  */
			streamParameters.iVideoHeight = tmpSize.iHeight;  /* iVideoParameters.iHeight; */
			streamParameters.iHaveAudio = ETrue;    //because u always insert silent amr frames atleast//aac frames
			if(iOutputAudioType == EAudioAMR)
				streamParameters.iAudioFormat = (CComposer::TAudioFormat)CComposer::EAudioFormatAMR;

			else if(iOutputAudioType == EAudioAAC)
				streamParameters.iAudioFormat = (CComposer::TAudioFormat)CComposer::EAudioFormatAAC;	//if amr out is amr else aac if none it will be none

		}
		else
		{
		    // No video, only audio; generate black frames. Can't use iParser->iStreamParameters since iParser doesn't exist
		    // SetHeaderDefaults() checked the resolution from movie to iVideoParameters
			streamParameters.iVideoWidth = iVideoParameters.iWidth;
			streamParameters.iVideoHeight = iVideoParameters.iHeight;
		}
	}
	else
	{	

		/* since all clips inserted are generated */
		iVideoType = iOutputVideoType;		

		VPASSERT( (iVideoType == EVideoH263Profile0Level10) ||
                  (iVideoType == EVideoH263Profile0Level45) ||
		          (iVideoType == EVideoMPEG4) ||
		          (iVideoType == EVideoAVCProfileBaseline) );		

		streamParameters.iCanSeek = ETrue;
		streamParameters.iNumDemuxChannels = 1; /* Because video will be there */
		TTimeIntervalMicroSeconds movduration = TTimeIntervalMicroSeconds(movie->Duration());
		TInt64 alllength = movduration.Int64();			
		streamParameters.iAudioLength = I64INT(alllength);
		streamParameters.iVideoLength = I64INT(alllength);
		streamParameters.iStreamLength = I64INT(alllength); 
		streamParameters.iVideoWidth = iVideoParameters.iWidth;
		streamParameters.iVideoHeight = iVideoParameters.iHeight;

		if (iOutputAudioType == EAudioAMR)
		{
			streamParameters.iAudioFormat = (CComposer::TAudioFormat) CComposer::EAudioFormatAMR; 
			streamParameters.iHaveVideo = ETrue;
			streamParameters.iNumDemuxChannels++;
			streamParameters.iAudioFramesInSample = KVedAudioFramesInSample;
			iAudioFramesInSample = KVedAudioFramesInSample;
		}

		else if (iOutputAudioType == EAudioAAC)
		{
			streamParameters.iAudioFormat = (CComposer::TAudioFormat) CComposer::EAudioFormatAAC; 
			streamParameters.iHaveVideo = ETrue;
			streamParameters.iNumDemuxChannels++;
			streamParameters.iAudioFramesInSample = 1;
			iAudioFramesInSample = 1;	// Same as above
		}

		else
		{
		
			streamParameters.iAudioFormat = (CComposer::TAudioFormat) CComposer::EAudioFormatNone; 
			streamParameters.iHaveAudio =EFalse;
			streamParameters.iAudioLength =0;	/* reset audio length as it was set to videolength */
			streamParameters.iAudioFramesInSample = KVedAudioFramesInSample;
			iAudioFramesInSample = KVedAudioFramesInSample;
		}
		
		streamParameters.iAudioTimeScale = KAMRAudioTimeScale;
		streamParameters.iVideoTimeScale = iImageVideoTimeScale;
		iOutputVideoTimeScale = iImageVideoTimeScale;
		iOutputAudioTimeScale = KAMRAudioTimeScale;

	}
	
    TAudFileProperties outProp = iMovie->Song()->OutputFileProperties();
    
    if (iMovie->Song()->ClipCount(KAllTrackIndices) > 0)
    {
        if ( outProp.iAudioType == EAudAAC_MPEG4)
        {
            iOutputAudioTimeScale = outProp.iSamplingRate;
               
        }    
    }    

	streamParameters.iVideoFormat = (CComposer::TVideoFormat)iOutputVideoType;
	streamParameters.iStreamBitrate = iMovie->VideoStandardBitrate();
	iComposer->ComposeHeaderL(streamParameters, iOutputVideoTimeScale, iOutputAudioTimeScale, iAudioFramesInSample);

	if( firstClipIsGenerated == 1 )
	{
		// since first clip is generated, destroy parser
		iMonitor->PrepareComplete();
		iMonitor->ProcessingStarted(iStartingProcessing);
		iStartingProcessing = EFalse;
		iState = EStateProcessing;
		
		// since first clip is generated, destroy parser
		if(iAllGeneratedClips == 1)
		{
			if(iParser)
			{	
				delete iParser;
				iParser =0;
			}
		}
	}
	    
    PRINT((_L("CMovieProcessorImpl::StartMovieL() end")))
	
}

// -----------------------------------------------------------------------------
// CMovieProcessorImpl::SetOutputMediaTypesL
// Set output audio/video types
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CMovieProcessorImpl::SetOutputMediaTypesL()
{

	CVedMovieImp* movie = (iMovie);

	if( movie->VideoType() == EVedVideoTypeH263Profile0Level10 )
		iOutputVideoType = EVideoH263Profile0Level10;

	else if ( movie->VideoType() == EVedVideoTypeH263Profile0Level45 )
		iOutputVideoType = 	EVideoH263Profile0Level45;

	else if ( movie->VideoType() == EVedVideoTypeMPEG4SimpleProfile )
		iOutputVideoType = EVideoMPEG4;
	
#ifdef VIDEOEDITORENGINE_AVC_EDITING
    else if ( movie->VideoType() == EVedVideoTypeAVCBaselineProfile )
		iOutputVideoType = EVideoAVCProfileBaseline;
#endif

	else 
		User::Leave(KErrArgument);

	CAudSong* songPointer = movie->Song();
	if ( songPointer->ClipCount(KAllTrackIndices) > 0 )
	    {
    	if( movie->AudioType() == EVedAudioTypeAMR )
    		iOutputAudioType = 	EAudioAMR;

    	else if ( movie->AudioType() == EVedAudioTypeAAC_LC )
    		iOutputAudioType = EAudioAAC;
    	else 
	    	User::Leave(KErrArgument);
	    }
    else
        {
        // no audio
        iOutputAudioType = EAudioNone;
        }

}

// -----------------------------------------------------------------------------
// CMovieProcessorImpl::GetTranscodeFactorsL
// Retrieve bitstream modes for input clips
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CMovieProcessorImpl::GetTranscodeFactorsL()
{

	if (!iNumberOfVideoClips)
		return;

	CVedMovieImp* movie = (iMovie);

	iThumbnailInProgress = ETrue;	

	InitializeClipStructuresL();

	for(TInt i = 0; i < movie->VideoClipCount(); i++)
	{
		CVedVideoClip* currentClip = movie->VideoClip(i);
		CVedVideoClipInfo* currentInfo = currentClip->Info();

		if( currentInfo->Class() == EVedVideoClipClassFile )
		{			
			TVedTranscodeFactor factor;			

            if ( currentInfo->FileHandle() )
            {
                iClipFileName.Zero();
                iClipFileHandle = currentInfo->FileHandle();
            }
            else
            {
                iClipFileHandle = NULL;
			    iClipFileName = currentInfo->FileName();
            }
			
			InitializeClipL(); // opens the file & parses header
			                   // open demux & decoder
			                   
            // Calculate the number of frames in the output clip                   
			iFrameParametersSize += iParser->GetNumberOfVideoFrames();			                   						

			iState = EStateProcessing;

			TInt error = iVideoProcessor->GetTranscodeFactorL(factor);

			if (error != KErrNone)
				User::Leave(error);
			
			if ( ((factor.iStreamType == EVedVideoBitstreamModeMPEG4Resyn)
			    || (factor.iStreamType == EVedVideoBitstreamModeMPEG4DP)
			    || (factor.iStreamType == EVedVideoBitstreamModeMPEG4DP_RVLC)
			    || (factor.iStreamType == EVedVideoBitstreamModeMPEG4Resyn_DP)
			    || (factor.iStreamType == EVedVideoBitstreamModeMPEG4Resyn_DP_RVLC))
			    && ( currentClip->Info()->Resolution() == movie->Resolution() )
			    && ( iOutputVideoType != EVideoAVCProfileBaseline ) )
			    {
			    // we do compressed domain transcoding for this clip, and it has 
			    // other mpeg4 modes than the regular; we need to ensure the VOS/VOL header
			    // has the resync marked flag enabled
			    iMpeg4ModeTranscoded = ETrue;
			    }

			currentInfo->SetTranscodeFactor(factor);

			DoCloseVideoL();  // close all
			
			if (iAvcEdit)
			{
			    delete iAvcEdit;
			    iAvcEdit = 0;
			}
		}
		else
		{
            // Calculate the number of frames in the output clip 
		    iFrameParametersSize += (TInt) currentInfo->VideoFrameCount();
		}
	}
	
	iThumbnailInProgress = EFalse;
	iState = EStateIdle;
}


// -----------------------------------------------------------------------------
// CMovieProcessorImpl::SetupTranscodingL
// Set video transcoding parameters
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CMovieProcessorImpl::SetupTranscodingL()
{

	if (!iNumberOfVideoClips)
		return;

	CVedMovieImp* tmovie = (iMovie);
	TBool transitionExists = EFalse;// Assume there are no middle transitions in any clip
	TBool cutExists = EFalse;		// Assuming there is no cut in any clip	
	TBool blackFrames = EFalse;     // Assume there is no need to encode black frames in the end
	TBool firstClipIsGen = EFalse;
	TBool firstClipIsFullTranscoded = EFalse;
	TBool clipFullTranscodingExists = EFalse;

	// Vos issues if first clip uses encoder
	TBool atleastOneH263 = EFalse;
	TBool atleastOneMPEG4 = EFalse;
//	TBool atleastOneAVC = EFalse;
	TBool differentModesExist = EFalse;

	// initially assume no mpeg4 files so no streammode
	TVedVideoBitstreamMode streamMode = EVedVideoBitstreamModeUnknown;
	TBool atleastOneGenerated = EFalse; // assuming that there are no generated clips
	TBool allGeneratedClips = ETrue; // asssuming all are generated
	iModeTranslationRequired = EFalse;  // no need for translation in all generated
	
	TSize outputVideoResolution = tmovie->Resolution();	// since movie resolution is minimum resolution

	for(TInt i = 0; i < tmovie->VideoClipCount(); i++)
	{
		CVedVideoClip* currentClip = tmovie->VideoClip(i);
		CVedVideoClipInfo* currentInfo = currentClip->Info();
		if( currentInfo->Class() == EVedVideoClipClassFile )
		{
			allGeneratedClips = EFalse;	// there is a file based clip
			
			if( (currentInfo->VideoType() == EVedVideoTypeH263Profile0Level10) ||
				(currentInfo->VideoType() == EVedVideoTypeH263Profile0Level45) )
			{
				// if there is even one H263 output is H263
				atleastOneH263 = ETrue;
			}
			else if(currentInfo->VideoType() == EVedVideoTypeAVCBaselineProfile)
			{
//				atleastOneAVC = ETrue;
			}
			else
			{
				if(currentInfo->VideoType() == EVedVideoTypeMPEG4SimpleProfile)	// if there is even one H263 output is H263
				{
					atleastOneMPEG4 = ETrue;
					if(streamMode == EVedVideoBitstreamModeUnknown)
					{	
						// since previously no mpeg4 was found to set streammode	
						streamMode = currentInfo->TranscodeFactor().iStreamType;
					} 
					else
					{
						if(streamMode != currentInfo->TranscodeFactor().iStreamType)  // different modes in Mpeg4
							differentModesExist = ETrue;
					}
				}
				else
				{
					//Error - improper or unsupported type file
					User::Leave(KErrNotSupported);
				}
			}
			
			// Here check if any clip is cut and also if first clip is cut
			TTimeIntervalMicroSeconds cutinTime = TTimeIntervalMicroSeconds(currentClip->CutInTime());
			if(cutinTime != TTimeIntervalMicroSeconds(0)) 
			{   
				// cut does exist so encoder will be used in at least one clip
				if(i==0) // which means cut exists in first clip itself
				{					
					iFirstClipIsCut = ETrue;
				}
				cutExists = ETrue;
			}

			// check if the clip will be full transcoded also as then we can 
			// decide whether to change VOS bit and to set ModeTranslation as all would be encoded again
			TSize currClipRes = currentInfo->Resolution();
			if(  ( (outputVideoResolution.iWidth != currClipRes.iWidth) && 
                   (outputVideoResolution.iHeight != currClipRes.iHeight) ) ||
                 ( iOutputVideoType == EVideoMPEG4 && currentInfo->VideoType() == EVedVideoTypeAVCBaselineProfile ) )
			{
				if(i == 0)
				{
					firstClipIsFullTranscoded = ETrue;  //for VOS bit change
				}
				clipFullTranscodingExists = ETrue;
			}
		}
		else
		{
			atleastOneGenerated = ETrue;
			if(i == 0) // then first clip is generated
			{
				firstClipIsGen = ETrue;				
			}
		}

		if( i != tmovie->VideoClipCount()-1 )
		{
			if(tmovie->MiddleTransitionEffect(i) != EVedMiddleTransitionEffectNone )					
			{											
				// this is required to check if any clips have
				// middle transitions so mode translation will be required	
				transitionExists = ETrue;  // even if all clips are of same mode
			}
		}
	}

	if( (TVedStartTransitionEffect)tmovie->StartTransitionEffect() != EVedStartTransitionEffectNone )
	{	
	    // even if all clips are of same mode but if it is different than what encoder uses transcoding is needed	
		transitionExists = ETrue;			
	}
	
	if( (TVedEndTransitionEffect)tmovie->EndTransitionEffect() != EVedEndTransitionEffectNone)
	{
	    // even if all clips are of same mode but if it is different than what encoder uses transcoding is needed
		transitionExists = ETrue;
	}
    if ( tmovie->Duration().Int64()/1000 > (tmovie->VideoClip(iNumberOfVideoClips-1)->EndTime().Int64()/1000) )
    {
        // movie is longer than video track => black frames are encoded in the end => even if all clips are of same mode but if it is different than what encoder uses transcoding is needed
        blackFrames = ETrue;
    }

	if(iOutputVideoType == EVideoMPEG4)	// if different modes dont exist then if output is Mpeg4
	{								
		PRINT((_L("CMovieProcessorImpl::SetupTranscodingL(), output type = MPEG-4")))

		if(differentModesExist)
		{	
			PRINT((_L("CMovieProcessorImpl::SetupTranscodingL(), different modes exist")))

			// if there are differnet mode Mpeg4's then mode translation is required		
			iModeTranslationRequired = ETrue;		// regardless of there being generated clips
		}
		else
		{
			PRINT((_L("CMovieProcessorImpl::SetupTranscodingL(), different modes don't exist")))

			if(atleastOneGenerated) // if there are any generated clips
			{							
				PRINT((_L("CMovieProcessorImpl::SetupTranscodingL(), at least one generated")))

				if(atleastOneMPEG4)		// if there is one Mpeg4 clip atleast	
				{
					PRINT((_L("CMovieProcessorImpl::SetupTranscodingL(), at least one MPEG-4")))

					if( (streamMode == EVedVideoBitstreamModeMPEG4Regular) ) //generated clips mode will be regular so u need to convert others 
					{	
						PRINT((_L("CMovieProcessorImpl::SetupTranscodingL(), streamMode is regular")))

						// if all are regular mode no need to change mode
						iModeTranslationRequired = EFalse;
					}
					else
					{									
						PRINT((_L("CMovieProcessorImpl::SetupTranscodingL(), streamMode != regular")))

						// to regular (with resync)
						iModeTranslationRequired = ETrue;
					}
				}
				else
				{
					if(atleastOneH263)
					{
						PRINT((_L("CMovieProcessorImpl::SetupTranscodingL(), at least one H.263")))
						iModeTranslationRequired = ETrue;
					}
					else
					{		
						PRINT((_L("CMovieProcessorImpl::SetupTranscodingL(), all are generated")))
						// all are generated no h263 or mpeg4
						iModeTranslationRequired = EFalse;
					}
				}
			}
			else
			{
				PRINT((_L("CMovieProcessorImpl::SetupTranscodingL(), no generated clips")))

				if(atleastOneH263)
				{
					PRINT((_L("CMovieProcessorImpl::SetupTranscodingL(), at least one H.263")))
					iModeTranslationRequired = ETrue;
				}
				else
				{
					PRINT((_L("CMovieProcessorImpl::SetupTranscodingL(), no H.263")))
					// still open if we have MPEG-4 nonregular + encoding
					iModeTranslationRequired = EFalse;
				}
			}
		}
	}
	else
	{  
		// output is H263 or AVC so mode translation not required
		iModeTranslationRequired = EFalse;
	}

	// make decision of mode translation based on whether there 
	// was a cut or transition or resolution transcoding in any clip

	if(iOutputVideoType == EVideoMPEG4)	// if output is Mpeg4
	{
		if ((!allGeneratedClips) && (streamMode != EVedVideoBitstreamModeMPEG4Regular)) // in case of differentModesExist, iModeTranslationRequired is already ETrue
	    {
		    // If we need to encode smth but not all (if all generated => no mix), encoding results in regular stream. 
		    // However, if input has nonregular, we need to translate the mode
			if(transitionExists || clipFullTranscodingExists || cutExists || blackFrames) 
    			{
				iModeTranslationRequired = ETrue;
	    		}
		}
		// make Decision for changing the Vos of the output movie based on whether the first frame would be encoded
		if(firstClipIsGen || firstClipIsFullTranscoded || ((TVedStartTransitionEffect)tmovie->StartTransitionEffect() != EVedStartTransitionEffectNone) || iFirstClipIsCut )
		{	
			iFirstClipUsesEncoder = ETrue;	// this indicates that you may need to change Vos bit but final decision is 
		}									// done based on whether it was due to cut on the fly
	}

}




// -----------------------------------------------------------------------------
// CMovieProcessorImpl::CreateImage3GPFilesL
// creates the necessary 3gp files from the given frames
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
TInt CMovieProcessorImpl::CreateImage3GPFilesL(TVideoOperation aCreateMode)
{   

    if (iProcessingCancelled)
        return KErrNone;
	
	if(aCreateMode == EVideoEncodeFrame) 
	{
        // encode frame		
		TTimeIntervalMicroSeconds inMicroSeconds = TTimeIntervalMicroSeconds(iVideoClip->Info()->Generator()->VideoFrameStartTime(iTotalImagesProcessed));		

		if (!IsActive())
		{
			SetActive();
			iStatus = KRequestPending;
		}
		iVideoEncoder->EncodeFrameL(iReadImageDes, iStatus, inMicroSeconds);

        // EncodeGiven
        iImageEncodedFlag = 1;
        iEncodeInProgress = 1;
		
		return KErrNone;		
	}
	else
	{	
        // a frame has been encoded, write it to output 3gp buffer

		// composer composing the movie
		if(iStatus == KErrNone)
		{
		    TBool isKeyFrame = 0;
			TPtrC8 buf(iVideoEncoder->GetBufferL(isKeyFrame));
			
			TBool modeChanged = EFalse;
			
			if ( iTotalImagesProcessed == 1 && iVideoType == EVideoMPEG4 )
			{
				modeChanged = ETrue;
			}
			
			/* composing is based on isIntra only */			
			TBool firstFrame = EFalse;
			if(iTotalImagesProcessed == 1)
			{
				firstFrame = ETrue;

				// VOS header size is parsed in composer for MPEG-4
	            iMP4SpecificSize = 0;
			}

			TInt64 durntTest = iVideoClip->Info()->Generator()->VideoFrameDuration(iTotalImagesProcessed-1).Int64();
			TInt64 durntsix = TInt64(((I64REAL(durntTest)/(TReal)1000)*(TReal) iImageVideoTimeScale) +0.5);
			TInt durnt = I64INT(durntsix);
			/* converting to ticks */
			durnt = (durnt)/1000;
            
            iGeneratedProcessed += durntTest/1000;            
			IncProgressBar();	/* indicate to gui about progress */

			iImageComposer->WriteFrames((TDesC8&)buf, buf.Size(), durnt, isKeyFrame ,
				1/*numberOfFrames*/, CMP4Composer::EFrameTypeVideo, iMP4SpecificSize,modeChanged,
				firstFrame,iVideoClip->Info()->TranscodeFactor().iStreamType, ETrue);				
			
			iVideoEncoder->ReturnBuffer();
		}
		/* end composing */
		
		return KErrNone;
	}
}

// -----------------------------------------------------------------------------
// CMovieProcessorImpl::ProcessImageSetsL
// Prepares for creating the necessary 3gp files from the given frames
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
TInt CMovieProcessorImpl::ProcessImageSetsL(TVideoOperation aCreateMode)
{

    if (iProcessingCancelled)
        return KErrNone;

	if(iTotalImagesProcessed == 0 && iImageEncodeProcFinished == 0)
	{		
		TFileName outputFileName = TPtrC(KTempFilePath);
		iCurrentMovieName = outputFileName;
		iCurrentMovieName.Append( _L( "Im" ) );
		iCurrentMovieName.Append( _L( "_nokia_vpi.tmp" ) );
		
#ifdef _DEBUG
        if (iOutputVideoType == EVideoAVCProfileBaseline)
		    VPASSERT(iImageAvcEdit);
#endif

		iImageComposer = CMP4Composer::NewL(iCurrentMovieName, (CParser::TVideoFormat)iOutputVideoType,
			                                CParser::EAudioFormatNone,
			                                iImageAvcEdit);

		CComposer::TStreamParameters innerStreamParameters;
		/* there will be no video clips initially, so initialize to default parameters of movie */
		TSize innerRes =(TSize)iVideoClip->Info()->Resolution(); /* resolution from generator */
		innerStreamParameters.iVideoWidth = innerRes.iWidth;      /* iVideoParameters.iWidth */
		innerStreamParameters.iVideoHeight = innerRes.iHeight;    /* iVideoParameters.iHeight */;
		/* width and height are initialised to the proper values from the clip */
		innerStreamParameters.iStreamBitrate = 25000 /* iStreamBitrate */;
		
		/* set the duration of the video clip */
		TTimeIntervalMicroSeconds iTempVideoLength= TTimeIntervalMicroSeconds(iVideoClip->Info()->Duration());
		TInt64  iTimeInMicro = (iTempVideoLength.Int64()/1000);
		innerStreamParameters.iVideoLength= I64INT(iTimeInMicro);       /* set the video length properly */
		innerStreamParameters.iStreamLength= I64INT(iTimeInMicro);
		innerStreamParameters.iAudioLength = 0;                         
		innerStreamParameters.iAudioFormat = (CComposer::TAudioFormat)0;
		CVedMovieImp* tempm = (iMovie);    

		if(iOutputVideoType == EVideoMPEG4)		
		{
			/* initialize to default constants for generated clips in case of MPEG-4 */
			innerStreamParameters.iVideoFormat = (CComposer::TVideoFormat) CComposer::EVideoFormatMPEG4;
			TVedTranscodeFactor tempFact;
			tempFact.iStreamType = EVedVideoBitstreamModeMPEG4Regular;
			tempFact.iTRes = 29;
			CVedVideoClip* currentClip = tempm->VideoClip(iVideoClipNumber);
			CVedVideoClipInfo* currentInfo = currentClip->Info();
			currentInfo->SetTranscodeFactor(tempFact);			/* set to default values, as initialized above */
		}
		else if ( (iOutputVideoType == EVideoH263Profile0Level10) ||
				  (iOutputVideoType == EVideoH263Profile0Level45) )
		{
		
			innerStreamParameters.iVideoFormat = (CComposer::TVideoFormat)iOutputVideoType;
		    /* initialize to default constants for generated clips in case of H.263 */
			TVedTranscodeFactor tempFact;
			tempFact.iStreamType = EVedVideoBitstreamModeH263;
			tempFact.iTRes = 0;
			CVedVideoClip* currentClip = tempm->VideoClip(iVideoClipNumber);
			CVedVideoClipInfo* currentInfo = currentClip->Info();
			currentInfo->SetTranscodeFactor(tempFact);		/* set to default values, as initialized above */
        } 
        
#ifdef VIDEOEDITORENGINE_AVC_EDITING        
        else if (iOutputVideoType == EVideoAVCProfileBaseline)
        {
        	/* initialize to default constants for generated clips in case of AVC */
			innerStreamParameters.iVideoFormat = (CComposer::TVideoFormat) CComposer::EVideoFormatAVCProfileBaseline;
			TVedTranscodeFactor tempFact;
			tempFact.iStreamType = EVedVideoBitstreamModeAVC;
			tempFact.iTRes = 30;
			CVedVideoClip* currentClip = tempm->VideoClip(iVideoClipNumber);
			CVedVideoClipInfo* currentInfo = currentClip->Info();
			currentInfo->SetTranscodeFactor(tempFact);			/* set to default values, as initialized above */
		}
#endif
        else
		    User::Leave(KErrNotSupported);


		if(iAllGeneratedClips == 1)
        {
			innerStreamParameters.iVideoWidth = iVideoParameters.iWidth;
			innerStreamParameters.iVideoHeight = iVideoParameters.iHeight;
		}
		innerStreamParameters.iCanSeek = ETrue;
		innerStreamParameters.iHaveVideo = ETrue;
		innerStreamParameters.iHaveAudio =EFalse;
		innerStreamParameters.iAudioFramesInSample =0;
		innerStreamParameters.iAudioTimeScale =KAMRAudioTimeScale; /* 8000 */
		innerStreamParameters.iVideoTimeScale =iImageVideoTimeScale;
		iImageComposer->ComposeHeaderL(innerStreamParameters,iImageVideoTimeScale /*iOutputVideoTimeScale*/,
			    iOutputAudioTimeScale, iAudioFramesInSample);
				
		if(!iVideoEncoder)
		{	
			/* It should never come here as iVideoEncoder is created before in hand */
			PRINT(_L("ERROR I VIDEOENCODER DOES NOT EXIST"));
			return 1; /* Indicating error */
		}
		else
		{
			//iVideoEncoder->Start();	/* make sure it is started only once */
		}
		
		TInt erInitialize = CreateImage3GPFilesL(aCreateMode);
		iTotalImagesProcessed++; 
		return erInitialize;
	}
	else if (aCreateMode == EVideoEncodeFrame && iImageEncodeProcFinished == 0)
	{
		/* for encoding, you will read from file, so increment the number of images processed */
		TInt er = CreateImage3GPFilesL(aCreateMode);
		/* before incrementing the number of images processed, check if it has actually been encoded */
		
		iTotalImagesProcessed++;
		
		return er;
	}
	else if(aCreateMode == EVideoWriteFrameToFile && iImageEncodeProcFinished == 0)
	{
		// a frame has been encoded, write it to output 3gp buffer
		TInt er2 = CreateImage3GPFilesL(aCreateMode);
		return er2;
	}
	else if(aCreateMode == EVideoWriteFrameToFile  && iImageEncodeProcFinished == 1)
	{
		return KErrGeneral; /* This should not happen */
	}
	else
	{
		/* This should not happen */
		return KErrGeneral;
	}
}


// -----------------------------------------------------------------------------
// CMovieProcessorImpl::EncodeImageFrameL
// Encodes raw frames for 3gp generated clips
// The frame is in buffer pointed to by iReadImageDes.
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
TInt CMovieProcessorImpl::EncodeImageFrameL()
{

    if (iProcessingCancelled)
        return KErrNone;

	iVideoEncoder->Stop();

	TTimeIntervalMicroSeconds inMSeconds = TTimeIntervalMicroSeconds(iVideoClip->Info()->Generator()->VideoFrameStartTime(iTotalImagesProcessed));
	
	if (!IsActive())
	{
		SetActive();
		iStatus = KRequestPending;
	}
	iVideoEncoder->EncodeFrameL(iReadImageDes, iStatus,inMSeconds);

    iTotalImagesProcessed++; /* Now we have encoded, and previously we had not, so increment now */
    iImageEncodedFlag = 1;
    iEncodeInProgress = 1;	   /* set to indicate encoding in progress */
    
	return KErrNone;
}

// -----------------------------------------------------------------------------
// CMovieProcessorImpl::ProcessImageSetsL
// Decides whether to encode or request for a frame from generator, 
// and in what mode to call the CreateImageFiles function
// The encoding is done calling the GetFrame, so it goes through the frame generator
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CMovieProcessorImpl::DoImageSetProcessL()
{

    if (iProcessingCancelled)
        return;

    // we come here from RunL

	if(iImageEncodedFlag == 0 && iImageEncodeProcFinished == 0)
    {
        // Starting to process an image => issue GetFrame() -request

		if(iFirstTimeProcessing == 0) 
		{				
			// this will be known from a bit or flag from the video clip		
			iFirstTimeProcessing = 1;  // this indicates that this is the first time we are getting info about this video clip
			iTotalImagesProcessed = 0;
			iNumOfImages = (TInt)iVideoClip->Info()->VideoFrameCount();
		}
		TSize tempres = iMovie->Resolution();
		if(iTotalImagesProcessed < iNumOfImages)
		{
			iGetFrameInProgress = 1; // indicates that the GetFrame was called and will be in progress
			iVideoClip->Info()->Generator()->GetFrameL(*this, /*0*/ iTotalImagesProcessed,&tempres, EColor4K, EFalse, 1);
		}
		else
		{
			// It should never come here though
			iImageClipCreated = 1;
		}
	}
	else if(iImageEncodedFlag == 1 && iImageEncodeProcFinished == 0)
	{
        // image has been encoded

		// tell the function to compose and return
		iEncodeInProgress = 0;
		iGetFrameInProgress = 0; // finished getting the frame, so if there's a cancel, no need to delete bitmap etc 
		ProcessImageSetsL(EVideoWriteFrameToFile);    // composing of VedVideoClipgenerator, though its not used inside 
		if(iNumOfImages == iTotalImagesProcessed)    /// if all images are over
		{
			iImageClipCreated = 1;
			iImageEncodeProcFinished = 1; // finished creating the image 3GP file, so go ahead */
            iImageComposer->Close();

			delete iImageComposer;
			iImageComposer = 0;

			if(iParser)
			{		
				delete iParser;
				iParser = 0;
			}

			/* set constant file name used as buffer */					
            iClipFileName = TPtrC(KTempFilePath);
			iClipFileName.Append( _L("Im_nokia_vpi.tmp") );            
			
			if (iImageAvcEdit)
			{
			    delete iImageAvcEdit;
			    iImageAvcEdit = 0;
			}
			
			if (iVideoEncoder)
			{
			    iVideoEncoder->Stop();
			    delete iVideoEncoder;
			    iVideoEncoder = 0;    
			}

			InitializeGeneratedClipL();
			/* reset the number of images for the next image set */
			iImageEncodedFlag = 0;

			if(!IsActive())
			{
				SetActive();			  // wait till the video encoder finishes initialising
				iStatus = KRequestPending;
			}
			User::Free(iYuvImageBuf);
			iYuvImageBuf = 0;
		}
		else
		{
            // request for a new frame

			User::Free(iYuvImageBuf);
			iYuvImageBuf = 0;
			TSize tempres = iMovie->Resolution();
			iGetFrameInProgress = 1;
			iVideoClip->Info()->Generator()->GetFrameL(*this,iTotalImagesProcessed,&tempres,EColor4K,EFalse,1);
		}
	}
}
	



// -----------------------------------------------------------------------------
// CMovieProcessorImpl::Reset
// Resets the processor for processing a new movie
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CMovieProcessorImpl::ResetL()
{

    // delete clip structures
    DeleteClipStructures();
        
    // delete video processing modules
    iEncoderInitPending = EFalse;
    iState = EStatePreparing;
    DoCloseVideoL();
    iState = EStateIdle;

    VPASSERT(!iEncoderInitPending);
    VPASSERT(!iVideoEncoder);

    if (iComposer) 
    {
        delete iComposer; 
        iComposer = 0;
    }         
        
    if(iImageComposer) 
	{
		delete iImageComposer;						
		iImageComposer=0;
	}
	
	if (iImageAvcEdit)
	{
	    delete iImageAvcEdit;
	    iImageAvcEdit = 0;
	}    
	
	if (iYuvImageBuf)
	{
		User::Free(iYuvImageBuf);
		iYuvImageBuf=0;
	}
	
	// for transition effect
    if ( iFsConnected )
    {
        CloseTransitionInfoL();
        iFs.Close();
        iFsConnected = EFalse;
    }
    
	iDataFormat = EDataAutoDetect;
	iStreamBitrate = 0;
	iNumDemuxChannels = 0;
	iOutputNumberOfFrames = 0;
	iVideoType = EVideoH263Profile0Level10;
	iAudioType = EAudioAMR;	
    iFirstFrameOfClip = EFalse;
    iFirstFrameFlagSet = EFalse;

    iFirstClipUsesEncoder = EFalse;
    iMpeg4ModeTranscoded = EFalse;
    
    iOutputVideoTimeScale = KVideoTimeScale;
	iOutputAudioTimeScale = KAMRAudioTimeScale;    
	iOutputVideoType = EVideoNone;
	iOutputAudioType = EAudioAMR;

	iProcessingCancelled = EFalse;

    iStartTransitionEffect = EVedStartTransitionEffectNone; 
	iMiddleTransitionEffect = EVedMiddleTransitionEffectNone; 
	iPreviousMiddleTransitionEffect = EVedMiddleTransitionEffectNone;
	iEndTransitionEffect = EVedEndTransitionEffectNone;
    iWriting1stColorTransitionFrame = EFalse;
    i1stColorTransitionFrameTS = 0;

    iApplySlowMotion = ETrue;
	iCurrentVideoTimeInTicks = 0.0; 
    iInitialClipStartTimeStamp = 0;
    iStartingProcessing = EFalse;
    iFramesProcessed = 0;	
    iProgress = 0;
    iGeneratedProcessed = 0;
    iAudioProcessingCompleted = EFalse;
    iEncodingBlackFrames = EFalse;
    
    iTotalDurationInSample = 0;
    iAudioProcessingCancelled = EFalse;
    iWaitSchedulerStarted = EFalse;
    	
	iCurrentMovieName.Zero();
	iAudioClipWritten = 0;
    iVideoClipWritten = 0;
    iDiskFull = EFalse;
    iAudioFrameNumber = 0;
    iVideoFrameNumber = 0;
    iFrameBuffered = EFalse;
    iVideoIntraFrameNumber = 0;
	iVideoClipNumber=0; 		
    iStartFrameIndex = 0;
	iVideoClip=0; 
	iMovie=0;	
	iSpeed = KMaxVideoSpeed; 
	iColorEffect = EVedColorEffectNone;
	iNumberOfVideoClips=0;
    iEncoderBuffer = 0;
    iEncodePending = 0;    
	iVideoClipDuration = 0;
	iLeftOverDuration = 0;
	iTimeStamp = 0;

    iThumbnailInProgress=EFalse; 
	iTotalMovieDuration = 0;
	iFramesProcessed=0;
	iStartCutTime = TTimeIntervalMicroSeconds(0); 
	iEndCutTime = TTimeIntervalMicroSeconds(0);

    iTr.iTrPrevNew = -1;
	iTr.iTrPrevOrig = -1;

    iAudioFramesInSample = KVedAudioFramesInSample;
    iAudioFramesInBuffer = 0;
    iOutAudioBuffer=0;
    iNumberOfAudioClipsCreated = 0;
    iCurrentAudioTimeInMs = 0;
	iTotalAudioTimeWrittenMs = 0;
	iNumberOfAudioClips=0;
	iAudioClipNumber=0;
	iTimeStampListScaled = EFalse;
	
	iCurrentVideoSize = 0; 
	iCurrentAudioSize = 0;
	iMovieSizeLimitExceeded = EFalse;
	
    iAllVideoProcessed = EFalse;
    
    // We are now properly initialized
    iState = EStateIdle;
}


// -----------------------------------------------------------------------------
// CMovieProcessorImpl::CancelProcessingL
// Stops processing a movie
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CMovieProcessorImpl::CancelProcessingL()
{	
    
    PRINT((_L("CancelProcessingL begin, iEncoderInitPending %d, iEncodePending %d, iEncodeInProgress %d"), 
				iEncoderInitPending, iEncodePending, iEncodeInProgress ));

#ifdef _DEBUG
	if (iVideoEncoder)
		PRINT((_L("CancelProcessingL() - iEncodePending in encoder %d"), iVideoEncoder->IsEncodePending()));
#endif

    if (iProcessingCancelled)
    {
    	PRINT((_L("CancelProcessingL() - Already cancelled!")));
    	
        if (iMonitor)
            iMonitor->ProcessingCancelled();
        
    	return;
    }
    
    iProcessingCancelled = ETrue;

    if (iDemux)
        iDemux->Stop();

    if (iVideoProcessor)
        iVideoProcessor->Stop();   
    
    if (iAudioProcessor)
        iAudioProcessor->StopL();                

    // delete all objects except status monitor
    DoCloseVideoL();
    
    if ( iVideoEncoder && iVideoEncoder->IsEncodePending() == 0 && 
	     (iEncodePending || iEncodeInProgress) )
	{
	    // encoder has completed encoding request, but scheduler has
	    // not called RunL() yet. Reset flags so that the request will	    
	    // be handled as init complete in RunL	    
	    PRINT((_L("CancelProcessingL() - resetting encoding flags")));
	    iEncoderInitPending = ETrue;
	    iEncodePending = iEncodeInProgress = 0;
	}
	
    // close the rest

	// for transition effect
    if ( iFsConnected )
    {
        CloseTransitionInfoL();
        iFs.Close();
        iFsConnected = EFalse;
    }

	if(iGetFrameInProgress == 1)
	{
		//VPASSERT(iEncodeInProgress == 0);
		iGetFrameInProgress = 0;
		
		if(iVideoClip->Info()->Class() == EVedVideoClipClassGenerated)
		{
			iVideoClip->Info()->Generator()->CancelFrame();
		}		
	}
	
	if (iComposer)
	{
		iComposer->Close();  // creates the output file                
		delete iComposer; 
		iComposer = 0;
	}		
	
	if(iImageComposer) 
	{
		delete iImageComposer;
		iImageComposer=0;
	}
	
	if (iImageAvcEdit)
	{
	    delete iImageAvcEdit;
	    iImageAvcEdit = 0;
	}
	
	
	DeleteClipStructures();

    if (!iEncoderInitPending)
    {
    	
    	PRINT((_L("CMovieProcessorImpl::CancelProcessingL - calling cancelled callback")));
    	// if StartMovieL() has not been called at this point,
    	// there is no status monitor or observer to call
    	if (iMonitor)
            iMonitor->ProcessingCancelled();		

    	iState = EStateIdle;	
    }

	PRINT((_L("CMovieProcessorImpl::CancelProcessingL end")))
    
}


// -----------------------------------------------------------------------------
// CMovieProcessorImpl::GetClipPropertiesL
// Retrieves parameters for a clip
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CMovieProcessorImpl::GetClipPropertiesL(const TDesC& aFileName, 
                                             RFile* aFileHandle,
                                             TVedVideoFormat& aFormat,
                                             TVedVideoType& aVideoType,
                                             TSize& aResolution,
                                             TVedAudioType& aAudioType,
                                             TTimeIntervalMicroSeconds& aDuration,
                                             TInt& aVideoFrameCount,
											 TInt& aSamplingRate, 
											 TVedAudioChannelMode& aChannelMode)
{	

    PRINT((_L("CMovieProcessorImpl::GetClipPropertiesL() begin")))

    TInt error = KErrNone;
    if (!aFileHandle)
    {        
    	// Check that 3gp file exists. 
        
        RFs fs;
        User::LeaveIfError(fs.Connect());

    	RFile file;
    	error = file.Open(fs, aFileName, EFileShareReadersOnly | EFileStream | EFileRead);
        if ( error != KErrNone )
        {        
            error = file.Open(fs, aFileName, EFileShareAny | EFileStream | EFileRead);     
        }
    	if (error == KErrNone)
    		{
    		file.Close();
    		}
        fs.Close();
    	User::LeaveIfError(error);    	        
    	
    	// get filename
	    iClipFileName = aFileName;
	    iClipFileHandle = NULL;
    } 
    else
    {
        iClipFileHandle = aFileHandle;
        iClipFileName.Zero();
    }
    	 	
	CParser::TStreamParameters iStreamParams;	
		
	// parse header 
	TRAP(error, ParseHeaderOnlyL(iStreamParams, iClipFileName, iClipFileHandle));

    if (error != KErrNone && error != KErrNotSupported)
        User::Leave(error);
	
	/* pass back clip properties */

	// video format (file format actually)
	if (iStreamParams.iFileFormat == CParser::EFileFormat3GP)
		aFormat = EVedVideoFormat3GPP;
	else if (iStreamParams.iFileFormat == CParser::EFileFormatMP4)
		aFormat = EVedVideoFormatMP4;
	else
		aFormat = EVedVideoFormatUnrecognized;

	// video type	
	if(iStreamParams.iVideoFormat == CParser::EVideoFormatNone)
		aVideoType = EVedVideoTypeNoVideo;
	else if (iStreamParams.iVideoFormat == CParser::EVideoFormatH263Profile0Level10)
		aVideoType = EVedVideoTypeH263Profile0Level10;
	else if (iStreamParams.iVideoFormat == CParser::EVideoFormatH263Profile0Level45)
		aVideoType = EVedVideoTypeH263Profile0Level45;
	else if(iStreamParams.iVideoFormat == CParser::EVideoFormatMPEG4)
		aVideoType = EVedVideoTypeMPEG4SimpleProfile;
	else if(iStreamParams.iVideoFormat == CParser::EVideoFormatAVCProfileBaseline)
		aVideoType = EVedVideoTypeAVCBaselineProfile;
	else
		aVideoType = EVedVideoTypeUnrecognized;
	
	// audio type
	if(!iStreamParams.iHaveAudio/*iStreamParams.iAudioFormat == CParser::EAudioFormatNone*/)
		aAudioType=EVedAudioTypeNoAudio;
	else if(iStreamParams.iAudioFormat == CParser::EAudioFormatAMR)
		aAudioType=EVedAudioTypeAMR;
	else if (iStreamParams.iAudioFormat == CParser::EAudioFormatAAC)
		aAudioType=EVedAudioTypeAAC_LC; // what about EVedAudioTypeAAC_LTP ???
	else
		aAudioType=EVedAudioTypeUnrecognized;

	// Dummy values, update when AAC support is there
	aSamplingRate = KVedAudioSamplingRate8k;
	aChannelMode = EVedAudioChannelModeSingleChannel;

	// resolution
	aResolution.iWidth = iStreamParams.iVideoWidth; 
	aResolution.iHeight = iStreamParams.iVideoHeight; 

	// common
    TUint duration = (iStreamParams.iVideoLength > iStreamParams.iAudioLength ? 
        iStreamParams.iVideoLength : iStreamParams.iAudioLength);
	aDuration = TTimeIntervalMicroSeconds( TInt64(duration) * TInt64(1000) );

	// get total number of video frames
	aVideoFrameCount = iParser->GetNumberOfVideoFrames();

	/***************IF Audio Type is AAC get the audio properties************************/
	
	if(iStreamParams.iAudioFormat == CParser::EAudioFormatAAC)
	{
		//temporarily initialize iOutputAudioType and iAudioType as AudioProcessor uses it
		iOutputAudioType = EAudioAAC;
		iAudioType = EAudioAAC;				
	}

    PRINT((_L("CMovieProcessorImpl::GetClipPropertiesL() end")))
}




// -----------------------------------------------------------------------------
// CMovieProcessorImpl::GenerateVideoFrameInfoArray
// Retrieves frames parameters for a clip to array
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CMovieProcessorImpl::GenerateVideoFrameInfoArrayL(const TDesC& aFileName, RFile* aFileHandle,
                                                       TVedVideoFrameInfo*& aVideoFrameInfoArray)
{

    PRINT((_L("CMovieProcessorImpl::GenerateVideoFrameInfoArray() begin")))

    TInt error;
    
    if  (!aFileHandle)
    {
    	// Check that 3gp file exists. 	
        RFs fs;
    	User::LeaveIfError(fs.Connect());

    	RFile file;
    	error = file.Open(fs, aFileName, EFileShareReadersOnly | EFileStream | EFileRead);
        if ( error != KErrNone )
        {        
            error = file.Open(fs, aFileName, EFileShareAny | EFileStream | EFileRead);     
        }
    	if (error == KErrNone)
    		{
    		file.Close();
    		}
    	fs.Close();
    	User::LeaveIfError(error);
    }
	 
	// parse clip header
	CParser::TStreamParameters streamParams;
	
	// get filename
	if (aFileHandle)
	{
	    iClipFileName.Zero();   
	    iClipFileHandle = aFileHandle;
	}
	else
	{	 
	    iClipFileHandle = NULL;
	    iClipFileName = aFileName;
	}
    
	// parse header 		
	TRAP(error, ParseHeaderOnlyL(streamParams, iClipFileName, iClipFileHandle));

    if (error != KErrNone && error != KErrNotSupported)
        {
		User::Leave( error );
	    }

	// video type
    TVedVideoType videoType = EVedVideoTypeNoVideo;
	if(streamParams.iVideoFormat == CParser::EVideoFormatNone)
		videoType = EVedVideoTypeNoVideo;
	else if(streamParams.iVideoFormat == CParser::EVideoFormatH263Profile0Level10)
		videoType = EVedVideoTypeH263Profile0Level10;
	else if(streamParams.iVideoFormat == CParser::EVideoFormatH263Profile0Level45)
		videoType = EVedVideoTypeH263Profile0Level45;
	else if(streamParams.iVideoFormat == CParser::EVideoFormatMPEG4)
		videoType = EVedVideoTypeMPEG4SimpleProfile;
	else if(streamParams.iVideoFormat == CParser::EVideoFormatAVCProfileBaseline)
		videoType = EVedVideoTypeAVCBaselineProfile;
	else 
	{
		User::Leave(KErrNotSupported);
	}	
	
	// frame parameters
	if( (videoType == EVedVideoTypeH263Profile0Level10) || 
		(videoType == EVedVideoTypeH263Profile0Level45) || 		
		(videoType == EVedVideoTypeMPEG4SimpleProfile)  ||
		(videoType == EVedVideoTypeAVCBaselineProfile) )
	{
		TInt frameCount = 0;
		FillVideoFrameInfoArrayL(frameCount, (TVedVideoFrameInfo*&)aVideoFrameInfoArray); 
	}

    PRINT((_L("CMovieProcessorImpl::GenerateVideoFrameInfoArray() end")))    
}

// -----------------------------------------------------------------------------
// CMovieProcessorImpl::FillVideoFrameInfoArray
// Fills an array containing video frame parameters: size, start time & type
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//

void CMovieProcessorImpl::FillVideoFrameInfoArrayL(TInt& aVideoFrameCount, 
												   TVedVideoFrameInfo*& aVideoFrameInfoArray)												  
{
	
	PRINT((_L("CMovieProcessorImpl::FillVideoFrameInfoArrayL() begin")))    

	// get total number of video frames
	aVideoFrameCount = iParser->GetNumberOfVideoFrames();
	VPASSERT(aVideoFrameCount); 
	// create memory for frame parameters - DO NOT delete it in MediaProcessorImpl
	if(aVideoFrameInfoArray)
	{
		delete aVideoFrameInfoArray;
		aVideoFrameInfoArray=0;
	}
		
	// get individual frame parameters
	aVideoFrameInfoArray = (TVedVideoFrameInfo*)User::AllocL(aVideoFrameCount * sizeof(class TVedVideoFrameInfo));
		
	TFrameInfoParameters* frameInfoArray = (TFrameInfoParameters*)User::AllocZL((aVideoFrameCount) * sizeof(struct TFrameInfoParameters));
	
	TInt i; 
	// Get all the frame parameters using the new function
	CMP4Parser* parser = (CMP4Parser*)iParser;
	TInt startIndex =0;
	TInt err = parser->GetVideoFrameProperties(frameInfoArray,startIndex,aVideoFrameCount);
	if(err !=0)
		User::Leave(KErrAbort);

	for(i=0; i<aVideoFrameCount; i++)
	{        
		aVideoFrameInfoArray[i].iSize = (TInt)frameInfoArray[i].iSize;
		aVideoFrameInfoArray[i].iStartTime = (TInt)frameInfoArray[i].iStartTime;
        if(frameInfoArray[i].iType)
            {
        	PRINT((_L("CMovieProcessorImpl::FillVideoFrameInfoArrayL() iType of %d nonzero, time %d"), i, aVideoFrameInfoArray[i].iStartTime))    
            aVideoFrameInfoArray[i].iFlags = 1;
            }
		else
            {
        	PRINT((_L("CMovieProcessorImpl::FillVideoFrameInfoArrayL() iType of %d zero, time %d"), i, aVideoFrameInfoArray[i].iStartTime))    
            aVideoFrameInfoArray[i].iFlags = 0;			
            }
	}	
	User::Free(frameInfoArray);
	frameInfoArray=0; 

	PRINT((_L("CMovieProcessorImpl::FillVideoFrameInfoArrayL() end")))    
	
}

// -----------------------------------------------------------------------------
// CMovieProcessorImpl::FillFrameParameters
// Fills an internal array containing parameters for each video frame : 
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CMovieProcessorImpl::FillFrameParametersL(TInt aCurrentFrameIndex)
{
	PRINT((_L("CMovieProcessorImpl::FillFrameParameters() begin, current index %d"),aCurrentFrameIndex))    

	// get total number of video frames
	TInt numberOfFrames = iParser->GetNumberOfFrames();
	TInt frameNumber = 0;
	
	// get start frame index in the input clip
	iStartFrameIndex = iParser->GetStartFrameIndex(); 

    TInt cutOutTime = 0;
    if (!iThumbnailInProgress)
        cutOutTime = I64INT( iVideoClip->CutOutTime().Int64() / TInt64(1000) );
    
    TFrameInfoParameters* frameInfoArray = 
        (TFrameInfoParameters*)User::AllocZL((numberOfFrames) * sizeof(struct TFrameInfoParameters));
    
    CleanupStack::PushL(frameInfoArray);
        
    // get info array from parser
    CMP4Parser* parser = (CMP4Parser*)iParser;    
    TInt error = parser->GetVideoFrameProperties(frameInfoArray, iStartFrameIndex, numberOfFrames);
    if (error != 0)
        User::Leave(KErrAbort);
    
    while ( frameNumber < numberOfFrames )
    {    
        iFrameParameters[aCurrentFrameIndex].iTimeStamp = 
            GetVideoTimeInTicksFromMs( TInt64(frameInfoArray[frameNumber].iStartTime), EFalse ); 
        iFrameParameters[aCurrentFrameIndex].iType = TUint8( frameInfoArray[frameNumber].iType );
        
        if (!iThumbnailInProgress && frameInfoArray[frameNumber].iStartTime > cutOutTime)
            {            
            break;
            }
        
        frameNumber++;
        aCurrentFrameIndex++;        
    }
        
    CleanupStack::PopAndDestroy(frameInfoArray);
    
	PRINT((_L("CMovieProcessorImpl::FillFrameParameters() end")))    
}


// -----------------------------------------------------------------------------
// CMovieProcessorImpl::IncProgressBar
// Report progress to observer
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//-
void CMovieProcessorImpl::IncProgressBar()
{
	VPASSERT(iTotalMovieDuration > 0);

	TInt64 msProcessed = iGeneratedProcessed + iTotalAudioTimeWrittenMs + 
        GetVideoTimeInMsFromTicks(iCurrentVideoTimeInTicks, ETrue);

	TInt percentage = TInt( ( (I64REAL(msProcessed) / I64REAL(iTotalMovieDuration)) * 100.0) + 0.5 );

	//VPASSERT( percentage <= 100 );

	if (percentage > iProgress && percentage <= 100)
	{
		iProgress = percentage;
		iMonitor->Progress(iProgress);
	}
}

// -----------------------------------------------------------------------------
// CMovieProcessorImpl::GetMovieSizeEstimateL
// Calculates an estimate for resulting movie size
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
TInt CMovieProcessorImpl::GetMovieSizeEstimateL(const CVedMovie* aMovie)
{
	TInt fileSize=0;
	iSizeEstimate->GetMovieSizeEstimateL(aMovie, (TInt&)fileSize);
	return fileSize;
}


// -----------------------------------------------------------------------------
// CMovieProcessorImpl::GetMovieSizeEstimateForMMSL
// Calculates file size estimate for MMS use
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
TInt CMovieProcessorImpl::GetMovieSizeEstimateForMMSL(const CVedMovie* aMovie, 
                                                      TInt aTargetSize, 
                                                      TTimeIntervalMicroSeconds aStartTime, 
                                                      TTimeIntervalMicroSeconds& aEndTime) 
{
    return iSizeEstimate->GetMovieSizeEstimateForMMSL(aMovie, aTargetSize, aStartTime, aEndTime);
} 



// -----------------------------------------------------------------------------
// CMovieProcessorImpl::StartThumbL
// Initiates thumbnail extraction from clip (full resolution raw is reutrned)
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CMovieProcessorImpl::StartThumbL(const TDesC& aFileName, 
                                      RFile* aFileHandle,
									  TInt aIndex, 
									  TSize aResolution,
									  TDisplayMode aDisplayMode, 
									  TBool aEnhance)
{

	PRINT((_L("CMovieProcessorImpl::StartThumbL() begin, aIndex = %d, enhance = %d"), aIndex, aEnhance))

    if (!aFileHandle)
    {
    	//Check that 3gp file exists. 
    	RFs fs;
    	User::LeaveIfError(fs.Connect());
    	RFile file;
    	TInt error = file.Open(fs, aFileName, EFileShareReadersOnly | EFileStream | EFileRead);
        if ( error != KErrNone )
        {        
            error = file.Open(fs, aFileName, EFileShareAny | EFileStream | EFileRead);
        }
    	if (error == KErrNone)
    		{
    		file.Close();
    		}
    	fs.Close();
    	User::LeaveIfError(error);
    }

	// get thumbnail parameters
	if (aFileHandle)
	{
	    iClipFileName.Zero();
	    iClipFileHandle = aFileHandle;
	}
	else
	{	 
	    iClipFileHandle = NULL;
	    iClipFileName = aFileName;     
	}

	iOutputThumbResolution.SetSize(aResolution.iWidth, aResolution.iHeight);
	iThumbIndex = aIndex;
	iThumbDisplayMode = aDisplayMode;
	iThumbEnhance = aEnhance;

	iThumbnailInProgress = ETrue;

	// initialization
	InitializeClipStructuresL();

	InitializeClipL(); // opens the file & parses header

    // update number of frames
	SetOutputNumberOfFrames(iParser->iOutputNumberOfFrames); 

	PRINT((_L("CMovieProcessorImpl::StartThumbL() end")))

}

// -----------------------------------------------------------------------------
// CMovieProcessorImpl::ProcessThumbL
// Generates thumbnail from clip (actually, full resolution raw is returned)
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CMovieProcessorImpl::ProcessThumbL(TRequestStatus &aStatus, TVedTranscodeFactor* aFactor)
{				
	
	PRINT((_L("CMovieProcessorImpl::ProcessThumbL() begin")))

    iState = EStateProcessing;	
    iThumbnailRequestStatus = &aStatus;
    
    // seek to the last intra frame before desired frame 
    TTimeIntervalMicroSeconds startTime(0);
    if ( iThumbIndex > 0 )
    {            
        TInt time = 0;
        TUint inMs = TUint( iParser->GetVideoFrameStartTime(iThumbIndex, &time) );
        TInt64 inMicroS = TInt64( inMs ) * TInt64( 1000 );            
        startTime = TTimeIntervalMicroSeconds( inMicroS );
    }
    
    // iOutputNumberOFrames contains the total amount of frames in clip
    // without cutting
    SetOutputNumberOfFrames(iParser->iOutputNumberOfFrames);
    
    TInt error = iParser->SeekOptimalIntraFrame(startTime, iThumbIndex, ETrue);
    if (error != KErrNone)
    {  
        iThumbnailRequestStatus = 0;
        User::Leave(KErrGeneral);
    }
    iStartFrameIndex = iParser->GetStartFrameIndex();
    VPASSERT(iStartFrameIndex >= 0);                  
  
    error = iVideoProcessor->ProcessThumb(this, iThumbIndex, iStartFrameIndex, aFactor);
    if (error != KErrNone)
    {
        iThumbnailRequestStatus = 0;
        User::Leave(error);
    }        
}
    

// -----------------------------------------------------------------------------
// CMovieProcessorImpl::NotifyThumbnailReady
// Called by thumbnail generator when thumbnail is ready
// for retrieval
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CMovieProcessorImpl::NotifyThumbnailReady(TInt aError)
{

    PRINT((_L("CMovieProcessorImpl::NotifyThumbnailReady() begin")))  

    if (HandleThumbnailError(aError))
        return;    
   
    // get YUV thumb
    iVideoProcessor->FetchThumb(&iYuvBuf);        
        
    // check validity of thumbnail and associated operation
    if(iThumbEnhance)		// for saving to file
    {
        if(iThumbDisplayMode == ENone)					// if no preference
            iThumbDisplayMode = EColor16M;				// 24-bit color image for enhancement
        else if(iThumbDisplayMode != EColor16M)	// invalid combination
        {     
            HandleThumbnailError(KErrNotSupported);
            return;                
        }
    }
    else								// for screen display
    {
        if(iThumbDisplayMode == ENone)					// if no preference
            iThumbDisplayMode = EColor64K;				// 16-bit image for some products
    }
        
    TInt bytesPerPixel = 0;
    // determine proper bit depth for the bitmap
    if(iThumbDisplayMode == EColor16M)
        bytesPerPixel = 3; // 24-bit rgb takes 3 bytes, stored as bbbbbbbb gggggggg rrrrrrrr
    else if(iThumbDisplayMode == EColor64K || iThumbDisplayMode == EColor4K)
        bytesPerPixel = 2; // 12-bit rgb takes 2 bytes, stored as ggggbbbb xxxxrrrr                            
    else
    {
        HandleThumbnailError(KErrNotSupported);
        return;					// support for 12-, 16- and 24-bit color images only
    }

    TInt error;
    if ( !iRgbBuf )
    {
        TSize inputFrameResolution(iParser->iStreamParameters.iVideoWidth,iParser->iStreamParameters.iVideoHeight);
                       
        // rgb specs
        TUint thumbLength = inputFrameResolution.iWidth * inputFrameResolution.iHeight; 
        TUint thumbUVLength = thumbLength>>2;	
        
        VPASSERT(iYuvBuf);
        // assign yuv pointers 
        TUint8* yBuf = iYuvBuf;
        TUint8* uBuf = yBuf + thumbLength;
        TUint8* vBuf = uBuf + thumbUVLength;	
                
        // create output rgb buffer
        TRAP(error, iRgbBuf = (TUint8*) User::AllocL(thumbLength * bytesPerPixel));
        if (HandleThumbnailError(error))
            return;                                
        
        TInt scanLineLength;
        
        // convert yuv to rgb
        switch (iThumbDisplayMode)
        {
            
        case EColor4K:
            {
                TInt error;
                CYuv2Rgb12* yuvConverter = NULL; 
                TRAP(error, yuvConverter = new(ELeave) CYuv2Rgb12);
                if (HandleThumbnailError(error))
                    return;
                scanLineLength = inputFrameResolution.iWidth * bytesPerPixel; 
                VPASSERT(yuvConverter);
                TRAP(error, yuvConverter->ConstructL(inputFrameResolution.iWidth, inputFrameResolution.iHeight, inputFrameResolution.iWidth, inputFrameResolution.iHeight));
                if (HandleThumbnailError(error))
                    return;
                yuvConverter->Convert(yBuf, uBuf, vBuf, inputFrameResolution.iWidth, inputFrameResolution.iHeight, iRgbBuf, scanLineLength);                
                delete yuvConverter;		
                yuvConverter=0;         
            }
            break;
            
        default:
        case EColor64K:
            {
                TInt error;
                CYuv2Rgb16* yuvConverter = NULL; 
                TRAP(error, yuvConverter = new(ELeave) CYuv2Rgb16);
                if (HandleThumbnailError(error))
                    return;                
                scanLineLength = inputFrameResolution.iWidth * bytesPerPixel; 
                VPASSERT(yuvConverter);
                TRAP(error, yuvConverter->ConstructL(inputFrameResolution.iWidth, inputFrameResolution.iHeight, inputFrameResolution.iWidth, inputFrameResolution.iHeight);)
                if (HandleThumbnailError(error))
                    return;
                yuvConverter->Convert(yBuf, uBuf, vBuf, inputFrameResolution.iWidth, inputFrameResolution.iHeight, iRgbBuf, scanLineLength);                
                delete yuvConverter;		
                yuvConverter=0; 
            }
            break;
            
        case EColor16M:
            {
                TInt error;
                CYuv2Rgb24* yuvConverter = NULL; 
                TRAP(error, yuvConverter = new(ELeave) CYuv2Rgb24);
                if (HandleThumbnailError(error))
                    return;                                
                scanLineLength = inputFrameResolution.iWidth * bytesPerPixel; 
                VPASSERT(yuvConverter);
                TRAP(error, yuvConverter->ConstructL(inputFrameResolution.iWidth, inputFrameResolution.iHeight, inputFrameResolution.iWidth, inputFrameResolution.iHeight))
                if (HandleThumbnailError(error))
                    return;                
                yuvConverter->Convert(yBuf, uBuf, vBuf, inputFrameResolution.iWidth, inputFrameResolution.iHeight, iRgbBuf, scanLineLength);                
                delete yuvConverter;		
                yuvConverter=0; 
            }
            break;
        }        
    }

    //CFbsBitmap* iOutBitmap = 0;

	if(!iThumbEnhance)
	{
        const TSize inputFrameResolution(iParser->iStreamParameters.iVideoWidth,iParser->iStreamParameters.iVideoHeight);

		/* Pre-calculate pixel indices for horizontal scaling. */
		// inputFrameResolution is the resolution of the image read from video clip.
		// iOutputThumbResolution is the final resolution desired by the caller.
		
		const TInt xIncrement = inputFrameResolution.iWidth * iOutputThumbResolution.iWidth;
		const TInt xBoundary = iOutputThumbResolution.iWidth * iOutputThumbResolution.iWidth;

		TInt* xIndices = 0;
		TRAPD(xIndicesErr, xIndices = new (ELeave) TInt[iOutputThumbResolution.iWidth]);
		if (xIndicesErr == KErrNone)
		{
			TInt xDecision = xIncrement / bytesPerPixel;
			TInt sourceIndex = 0;
			for (TInt x = 0; x < iOutputThumbResolution.iWidth; x++)
			{
				while (xDecision > xBoundary)
				{
					xDecision -= xBoundary;
					sourceIndex += bytesPerPixel;
				}

				xIndices[x] = sourceIndex;
				xDecision += xIncrement;
			}
		}
		else
		{		    
		    HandleThumbnailError(xIndicesErr);
		    return;
		}

		/* Initialize bitmap. */
		TRAPD(bitmapErr, iOutBitmap = new (ELeave) CFbsBitmap);
		if ((xIndicesErr == KErrNone) && (bitmapErr == KErrNone))
		{
			bitmapErr = iOutBitmap->Create(iOutputThumbResolution, iThumbDisplayMode /*EColor64K*/);
			if (bitmapErr == KErrNone)
			{
                // Lock the heap to prevent the FBS server from invalidating the address
                iOutBitmap->LockHeap();

				/* Scale to desired iOutputThumbResolution and copy to bitmap. */
				TUint8* dataAddress = (TUint8*)iOutBitmap->DataAddress();
				const TInt yIncrement = inputFrameResolution.iHeight * iOutputThumbResolution.iHeight;
				const TInt yBoundary = iOutputThumbResolution.iHeight * iOutputThumbResolution.iHeight;
				
				TInt targetIndex = 0;
				TInt sourceRowIndex = 0;
				TInt yDecision = yIncrement / 2;
				for (TInt y = 0; y < iOutputThumbResolution.iHeight; y++)
				{
					while (yDecision > yBoundary)
					{
						yDecision -= yBoundary;
						sourceRowIndex += (inputFrameResolution.iWidth * bytesPerPixel);
					}
					yDecision += yIncrement;
					
					for (TInt x = 0; x < iOutputThumbResolution.iWidth; x++)
					{
                        for (TInt i = 0; i < bytesPerPixel; ++i)
                        {
                            const TInt firstPixelSourceIndex = sourceRowIndex + xIndices[x] + i;
                            dataAddress[targetIndex] = iRgbBuf[firstPixelSourceIndex];
                            targetIndex++;
                        }
					}
				}
                iOutBitmap->UnlockHeap();
			}
			else
			{			    
				delete iOutBitmap; iOutBitmap = 0;
				HandleThumbnailError(bitmapErr);
				return;
			}
		}
		else
		{
		    HandleThumbnailError(bitmapErr);
		    delete[] xIndices; xIndices = 0;
		    return;
		}
		
		delete[] xIndices;
		xIndices = 0;
	}
	else		// enhance
	{
		TInt i,j;
		// create input bitmap and buffer
		CFbsBitmap* inBitmap = 0;
		TRAPD(inBitmapErr, inBitmap = new (ELeave) CFbsBitmap);
		if( inBitmapErr == KErrNone )
        {
		    // create bitmaps
		    TSize originalResolution(iParser->iStreamParameters.iVideoWidth, iParser->iStreamParameters.iVideoHeight);
		    inBitmapErr = inBitmap->Create(originalResolution, iThumbDisplayMode/*EColor16M*/); 
		
            if( inBitmapErr == KErrNone )
            {
		        // fill image from rgb buffer to input bitmap buffer 
		        TPtr8 linePtr(0,0); 
        		TInt lineLength = inBitmap->ScanLineLength(originalResolution.iWidth, iThumbDisplayMode); 
		        for(j=0, i=0; j<originalResolution.iHeight; j++, i+=lineLength)
		        {
        			linePtr.Set(iRgbBuf+i, lineLength, lineLength);
		        	inBitmap->SetScanLine((TDes8&)linePtr,j); 
		        }
		
        		// create output bitmap 
		        TRAPD(outBitmapErr, iOutBitmap = new (ELeave) CFbsBitmap);
                if( outBitmapErr == KErrNone )
                {
		            outBitmapErr = iOutBitmap->Create(iOutputThumbResolution, iThumbDisplayMode/*EColor16M*/); // same size as input frame
		
                    if( outBitmapErr == KErrNone )
                    {
		                // post-processing enhancement 
		                TRAP(outBitmapErr, EnhanceThumbnailL((const CFbsBitmap*)inBitmap, (CFbsBitmap*)iOutBitmap));

                    }
                    else
                    {
                        delete inBitmap; inBitmap = 0;   
                        delete iOutBitmap; iOutBitmap = 0;
                        HandleThumbnailError(outBitmapErr);
                        return;
                    }
                }
                else
                {
                     delete inBitmap; inBitmap = 0;
                     HandleThumbnailError(outBitmapErr);
                     return;
                }
            }
            else
            {
                delete inBitmap; inBitmap = 0;
                HandleThumbnailError(inBitmapErr);
                return;                
            }
		
		    // delete input bitmap 
		    delete inBitmap;
		    inBitmap = 0;
        }
        else
        {
            HandleThumbnailError(inBitmapErr);
            return;
        }
	}

    // return enhanced bitmap
    //aThumb = outBitmap;
    //iState = EStateReadyToProcess;  

    iYuvBuf = 0;
    delete iRgbBuf;
    iRgbBuf = 0;
    
    VPASSERT(iThumbnailRequestStatus);
    User::RequestComplete(iThumbnailRequestStatus, KErrNone);
	iThumbnailRequestStatus = 0;	

	PRINT((_L("CMovieProcessorImpl::NotifyThumbnailReady() end")))        
}


// -----------------------------------------------------------------------------
// CMovieProcessorImpl::HandleThumbnailError
// Handle error in thumbnail generation
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
TBool CMovieProcessorImpl::HandleThumbnailError(TInt aError)
{
    if (aError != KErrNone)
    {
        TInt error = aError;

#ifndef _DEBUG
        if (error < KErrHardwareNotAvailable)
            error = KErrGeneral;
#endif        
    
        VPASSERT(iThumbnailRequestStatus);
        User::RequestComplete(iThumbnailRequestStatus, error);
		iThumbnailRequestStatus = 0;
		return ETrue;		
    }                
    return EFalse;
}

// -----------------------------------------------------------------------------
// CMovieProcessorImpl::FetchThumb
// Returns a pointer to completed thumbnail bitmap
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CMovieProcessorImpl::FetchThumb(CFbsBitmap*& aThumb)
{
    aThumb = iOutBitmap;
    iOutBitmap = 0;
    
    iState = EStateReadyToProcess;
}

// -----------------------------------------------------------------------------
// CMovieProcessorImpl::InitializeClipStructuresL
// Initializes internal structures for movie processing
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CMovieProcessorImpl::InitializeClipStructuresL()
{

	TTimeIntervalMicroSeconds time;
    TInt i;
    
	// create memory for structures
	// VIDEO 
	if(!iVideoClipParameters)
	{
		iVideoClipParameters = (struct TVideoClipParameters *)User::AllocL(iNumberOfVideoClips *	
			sizeof(struct TVideoClipParameters));
		Mem::Fill(iVideoClipParameters, iNumberOfVideoClips*sizeof(TVideoClipParameters), 0);
	}	

	if(!iThumbnailInProgress)
	{
        // create audio buffer
        iOutAudioBuffer = (HBufC8*) HBufC8::NewL(KInitialAudioBufferSize);

        // create video buffer
        iOutVideoBuffer = (HBufC8*) HBufC8::NewL(KInitialVideoBufferSize);

        CVedMovieImp* movie = (iMovie);
		// initialize video clip parameters 
		for(i=0; i<iNumberOfVideoClips; i++)
		{
            // convert start & end times to milliseconds
			iVideoClip = movie->VideoClip(i);  
			time =	TTimeIntervalMicroSeconds(iVideoClip->StartTime());
			iVideoClipParameters[i].iStartTime = time.Int64()/1000; 
			time =	TTimeIntervalMicroSeconds(iVideoClip->EndTime());
			iVideoClipParameters[i].iEndTime = time.Int64()/1000; 
		}        
			
	}

	return;
}


// -----------------------------------------------------------------------------
// CMovieProcessorImpl::DeleteClipStructures
// Frees memory allocated for internal structures
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CMovieProcessorImpl::DeleteClipStructures()
{

    if (iFrameParameters)
		User::Free(iFrameParameters); 
    iFrameParameters = 0;
    iFrameParametersSize = 0;

	if (iVideoClipParameters)
		User::Free(iVideoClipParameters);	
    iVideoClipParameters = 0; 

    if (iEncoderBuffer)
        User::Free(iEncoderBuffer);
    iEncoderBuffer = 0;

    if (iOutAudioBuffer)
        delete iOutAudioBuffer;
	iOutAudioBuffer=0;

    if (iOutVideoBuffer)
        delete iOutVideoBuffer;
    iOutVideoBuffer = 0;

}


// -----------------------------------------------------------------------------
// CMovieProcessorImpl::InitializeClipL
// Initializes the processor for processing a clip
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CMovieProcessorImpl::InitializeClipL()
{
	
    PRINT((_L("CMovieProcessorImpl::InitializeClipL() begin")));

    iFirstFrameOfClip = ETrue;
    iFirstFrameFlagSet = EFalse;
	iModeChanged = EFalse;	    /* assuming that this clip's mode has not been changed	*/
    iVideoClipWritten = 0;
    iWriting1stColorTransitionFrame = EFalse;

 	// update clip properties
	if (iThumbnailInProgress)
	{
		// just get the first frame in normal mode		
		iSpeed        =	KMaxVideoSpeed;	
		iColorEffect  =	(TVedColorEffect)EVedColorEffectNone;	
		iStartCutTime =	0; 
		iEndCutTime   =	10;	
	}
	else
	{
        if (iNumberOfVideoClips) 
        {
            if ( iVideoClip->Info()->FileHandle() )
            {
                iClipFileName.Zero();
                iClipFileHandle = iVideoClip->Info()->FileHandle(); 
            }
            else
            {
                iClipFileHandle = NULL;
                iClipFileName =	(TPtrC)iVideoClip->Info()->FileName();
            }
            
            iSpeed        =	(TInt)iVideoClip->Speed();	
            iColorEffect  =	(TVedColorEffect)iVideoClip->ColorEffect();	
            iStartCutTime =	TTimeIntervalMicroSeconds(iVideoClip->CutInTime());
            iEndCutTime   =	TTimeIntervalMicroSeconds(iVideoClip->CutOutTime());
            
            iColorToneRgb = iVideoClip->ColorTone();
            ConvertColorToneRGBToYUV(iColorEffect,iColorToneRgb);
            // store previous middle transition, if there is more than one middle transition
            if(iVideoClipNumber > 0)
                iPreviousMiddleTransitionEffect = iMiddleTransitionEffect;
            // check if there is a position for middle transition for this clip
            if(iMovie->MiddleTransitionEffectCount() > iVideoClipNumber)
		{
                iMiddleTransitionEffect = iMovie->MiddleTransitionEffect(iVideoClipNumber);

				if( ( iMiddleTransitionEffect == EVedMiddleTransitionEffectCrossfade ||
					iMiddleTransitionEffect == EVedMiddleTransitionEffectWipeLeftToRight ||
					iMiddleTransitionEffect == EVedMiddleTransitionEffectWipeRightToLeft ||
					iMiddleTransitionEffect == EVedMiddleTransitionEffectWipeTopToBottom ||
					iMiddleTransitionEffect == EVedMiddleTransitionEffectWipeBottomToTop ) &&
                    ( iVideoClipNumber != iNumberOfVideoClips - 1 ) )
                {
                    TParse filename, filepath;
                    CVedMovieImp* movie = (iMovie);

					if( movie->VideoClip( iVideoClipNumber + 1 )->Info()->Class() == EVedVideoClipClassGenerated)
					{
						/************************************************************************/						
						TFileName ImageMovieName(KTempFilePath);						
						ImageMovieName.Append( _L( "Im_" ) );
						ImageMovieName.Append( _L( "nokia_vpi.tmp" ) );
						/************************************************************************/

						filename.Set( ImageMovieName, NULL, NULL );
						
                        if (iOutputFileHandle)
						{
						    RFile* file = iOutputFileHandle;
						    TFileName fullName;
                            TInt error = file->FullName(fullName);
                            filepath.Set(fullName, NULL, NULL);                            
						}
						else
						    filepath.Set( iOutputMovieFileName, NULL, NULL );
					}
					else{

                        if ( movie->VideoClip( iVideoClipNumber + 1 )->Info()->FileHandle() != NULL )
                        {
                            
                            RFile* file = movie->VideoClip( iVideoClipNumber + 1 )->Info()->FileHandle();
                            
                            TFileName origName;
                            TInt error = file->Name(origName);
                            filename.Set(origName, NULL, NULL);
                            
                            TFileName fullName;
                            error = file->FullName(fullName);
                            filepath.Set(fullName, NULL, NULL);
                            
                        } 
                        else
                        {                            
    						filename.Set( movie->VideoClip( iVideoClipNumber + 1 )->Info()->FileName(), NULL, NULL );
    						filepath.Set( iOutputMovieFileName, NULL, NULL );
                        }

					}	

					iNextClipFileName = filepath.DriveAndPath();
					iNextClipFileName.Append( filename.Name() );
					// VPI special tmp file
					iNextClipFileName.Append( _L( "_" ) );
					iNextClipFileName.AppendNum( iVideoClipNumber );
					iNextClipFileName.Append( _L( "_vpi.tmp" ) );
					// try to create a tmp file
                    if ( iNextClip.Create( iFs, iNextClipFileName, EFileStream | EFileWrite | EFileShareExclusive ) != KErrNone )
                    {
                        // check if the tmp file exists
                        if ( iNextClip.Open( iFs, iNextClipFileName, EFileStream | EFileWrite | EFileShareExclusive ) != KErrNone )
                        {
                            iNextClip.Close();
                            iNextClipFileName.Zero();
                        }
                    }
				}
			}
        }

        // this is in common timescale
        iInitialClipStartTimeStamp = (iVideoClipNumber>0 ? iVideoClipParameters[iVideoClipNumber-1].iEndTime : 0);
	}

    if (!iThumbnailInProgress)    
	    iMonitor->StartPreparing();

	// create an instance of the parser 
	if (iNumberOfVideoClips)
	{	

        if (!iParser) 
        {            
            if (iClipFileHandle)
                iParser = (CMP4Parser*) CMP4Parser::NewL(this, iClipFileHandle);
            else
                iParser = (CMP4Parser*) CMP4Parser::NewL(this, iClipFileName);
        }
        iParser->iFirstTimeClipParsing = ETrue;
        iState = EStateIdle;
        // open file & parse header
        CMovieProcessorImpl::TDataFormat format = CMovieProcessorImpl::EDataAutoDetect;        
        User::LeaveIfError(OpenStream(iClipFileName, iClipFileHandle, format));

		if ( iThumbnailInProgress && (iHaveVideo == EFalse) )
			User::Leave(KErrNotFound);

        VPASSERT(iState == EStateOpened);
	}
    else
        SetHeaderDefaults();
    
    iState = EStatePreparing;

    if (!iThumbnailInProgress)
    {

		// Since the clip does not have any audio type it can be over written by any audio type depending on output	
		if(iOutputAudioType == EAudioAMR)
		{
			if( iAudioType == EAudioNone)
				iAudioType = EAudioAMR;
		}
		else if(iOutputAudioType == EAudioAAC)
		{
			if( iAudioType == EAudioNone)
			{
				iAudioType = EAudioAAC;	
				if(iVideoClipNumber == 0)
				{
					//because it has no audio so it will have no decoder specific Info
					iFirstClipHasNoDecInfo = ETrue; 
				}
				iAudioFramesInSample = 1;
			}
		}               

        iEncoderInitPending = ETrue;        
        // complete request to finish initialising & start processing
		SetActive();			
		iStatus = KRequestPending;				
		
		TRequestStatus *status = &iStatus;        
        User::RequestComplete(status, KErrNone);

        if (iNumberOfVideoClips)
        {
			if (iParser)
			{
				// update the video clip duration in millisec.
				iVideoClipDuration = (TInt64)(iVideoClip->Info()->Duration().Int64() * 
					(TInt64)iParser->iStreamParameters.iVideoTimeScale / (TInt64)(1000000));
			}		
        }		
    }

    else 
    {        
        // open demux & decoder
        User::LeaveIfError(Prepare());

        VPASSERT(iState == EStateReadyToProcess);        
    }    

	PRINT((_L("CMovieProcessorImpl::InitializeClipL() end")))
}


// -----------------------------------------------------------------------------
// CMovieProcessorImpl::InitializeGeneratedClipL
// Initializes the processor for processing a generated clip
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CMovieProcessorImpl::InitializeGeneratedClipL()
{
	iEncodeInProgress = 0;
	iFirstFrameOfClip = ETrue; // initialized to indicate that the first frame of this clip has not yet been written
    iFirstFrameFlagSet = EFalse;
	iModeChanged = EFalse;	  /* assuming that this clip's mode has not been changed	*/	
    iVideoClipWritten = 0;
    iWriting1stColorTransitionFrame = EFalse;
	
	// update clip properties
	if (iThumbnailInProgress)
	{
		/* just get the first frame in normal mode */
		iSpeed        =	KMaxVideoSpeed;	
		iColorEffect  =	(TVedColorEffect)EVedColorEffectNone;	
		iStartCutTime =	0; 
		iEndCutTime   =	10;	
	}
	else
	{
		if (iNumberOfVideoClips) 
		{			
			iSpeed        =	(TInt)iVideoClip->Speed();	
			iColorEffect  =	iVideoClip->ColorEffect();	
			iStartCutTime =	TTimeIntervalMicroSeconds(0);         /* since generated clips cannot be cut */
            iEndCutTime   =	TTimeIntervalMicroSeconds(iVideoClip->Info()->Duration());
            
            iColorToneRgb = iVideoClip->ColorTone();
            ConvertColorToneRGBToYUV(iColorEffect,iColorToneRgb);
            
			/* store previous middle transition, if there is more than one middle transition */
			if(iVideoClipNumber > 0)
				iPreviousMiddleTransitionEffect = iMiddleTransitionEffect;
			/* since it is image clip, there will be no middletransitioneffect
			   check if there is a position for middle transition for this clip
			*/
			if(iMovie->MiddleTransitionEffectCount() > iVideoClipNumber)
			{
				iMiddleTransitionEffect = iMovie->MiddleTransitionEffect(iVideoClipNumber);            
				
				if( ( iMiddleTransitionEffect == EVedMiddleTransitionEffectCrossfade ||
					iMiddleTransitionEffect == EVedMiddleTransitionEffectWipeLeftToRight ||
					iMiddleTransitionEffect == EVedMiddleTransitionEffectWipeRightToLeft ||
					iMiddleTransitionEffect == EVedMiddleTransitionEffectWipeTopToBottom ||
					iMiddleTransitionEffect == EVedMiddleTransitionEffectWipeBottomToTop ) &&
					( iVideoClipNumber != iNumberOfVideoClips - 1 ) )
				{
					TParse filename, filepath;
					CVedMovieImp* movie = (iMovie);
					if( movie->VideoClip( iVideoClipNumber + 1 )->Info()->Class() == EVedVideoClipClassGenerated)
					{
						/* path for storing temporary files */						
						TFileName ImageMovieName(KTempFilePath);
						ImageMovieName.Append( _L( "Im_" ) );
						ImageMovieName.Append( _L( "nokia_vpi.tmp" ) );
						filename.Set( ImageMovieName, NULL, NULL );
						
						if (iOutputFileHandle)
						{
						    RFile* file = iOutputFileHandle;
						    TFileName fullName;
                            TInt error = file->FullName(fullName);
                            filepath.Set(fullName, NULL, NULL);                            
						}
						else
						    filepath.Set( iOutputMovieFileName, NULL, NULL );
					}
					else
					{
					    if ( movie->VideoClip( iVideoClipNumber + 1 )->Info()->FileHandle() != NULL )
                        {
                            RFile* file = movie->VideoClip( iVideoClipNumber + 1 )->Info()->FileHandle();
                            
                            TFileName origName;
                            TInt error = file->Name(origName);
                            filename.Set(origName, NULL, NULL);
                            
                            TFileName fullName;
                            error = file->FullName(fullName);
                            filepath.Set(fullName, NULL, NULL);                            
                        } 
                        else
                        {                            					
    						filename.Set( movie->VideoClip( iVideoClipNumber + 1 )->Info()->FileName(), NULL, NULL );
    						filepath.Set( iOutputMovieFileName, NULL, NULL );
                        }
					}
					
					iNextClipFileName = filepath.DriveAndPath();
					iNextClipFileName.Append( filename.Name() );
					/* tag it to indicate that its a tmp editor file */
					iNextClipFileName.Append( _L( "_" ) );
					iNextClipFileName.AppendNum( iVideoClipNumber );
					iNextClipFileName.Append( _L( "_vpi.tmp" ) );
					/* try to create a tmp file */
					if ( iNextClip.Create( iFs, iNextClipFileName, EFileStream | EFileWrite | EFileShareExclusive ) != KErrNone )
					{
						/* check if the tmp file exists */
						if ( iNextClip.Open( iFs, iNextClipFileName, EFileStream | EFileWrite | EFileShareExclusive ) != KErrNone )
						{
							iNextClip.Close();
							iNextClipFileName.Zero();
						}
					}
				}
			}
		}
		
		/* this is in common timescale */
		iInitialClipStartTimeStamp = (iVideoClipNumber>0 ? iVideoClipParameters[iVideoClipNumber-1].iEndTime : 0);
	}
	
	if (iNumberOfVideoClips)
	{		

		if (!iParser) /* if file name does not exist, this may cause error while parsing */
		{			
			iParser = (CMP4Parser*) CMP4Parser::NewL(this, iClipFileName);
		}
		else
		{			
			delete iParser;
			iParser = 0;
			iParser = (CMP4Parser*) CMP4Parser::NewL(this, iClipFileName);
		}

		iParser->iFirstTimeClipParsing = ETrue;
		iState = EStateIdle;
		/* open file and parse header */
		CMovieProcessorImpl::TDataFormat format = CMovieProcessorImpl::EDataAutoDetect;        
		/* this will be overloaded to accomodate for the buffer type */
		
		// HUOM! Meneekö tää oikein ??
		User::LeaveIfError(OpenStream(iClipFileName, NULL, format));
		VPASSERT(iState == EStateOpened);		

	}
	else
	{
		SetHeaderDefaults();
	}
	
	iState = EStatePreparing;
	if (!iThumbnailInProgress)
	{	
	
	    iEncoderInitPending = ETrue;        
        // async.			
		if (!IsActive())
		{
			SetActive();
			iStatus = KRequestPending;
		}
		TRequestStatus *status = &iStatus;
        User::RequestComplete(status, KErrNone);
		
		if (iNumberOfVideoClips)
		{
			/* update the video clip duration in millisec. */
			iVideoClipDuration = (TInt64)(iVideoClip->Info()->Duration().Int64() * 
				(TInt64)iParser->iStreamParameters.iVideoTimeScale / (TInt64)(1000000)); 
		}

		if(iOutputAudioType == EAudioAMR)
		{	// setting generated clip type same as output type
			if( iAudioType == EAudioNone)
			   iAudioType = EAudioAMR;
		}else if(iOutputAudioType == EAudioAAC)
		{
			if( iAudioType == EAudioNone)
				iAudioType = EAudioAAC;	
		}

	}
	else      /* if thumbnail is in progress */
	{        
		/* open demux and decoder */
		User::LeaveIfError(Prepare());
		VPASSERT(iState == EStateReadyToProcess);        
	}    
}


// -----------------------------------------------------------------------------
// CMovieProcessorImpl::TemporaryInitializeGeneratedClipL
// temporarily initializes the processor for image 3gp file creation
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CMovieProcessorImpl::TemporaryInitializeGeneratedClipL(){
	
	/* re-initialize to create new clip */
	iFirstFrameOfClip = ETrue;  /* initialized to indicate that the first frame of this clip has not yet been written */
    iFirstFrameFlagSet = EFalse;
	iModeChanged = EFalse;	   /* assuming that this clip's mode has not been changed	*/	
	iImageClipCreated = 0;
	iWriting1stColorTransitionFrame = EFalse;
	
	iImageEncodedFlag = 0;	// has an image been encoded
	iImageEncodeProcFinished = 0;	
	iFirstTimeProcessing = 0;
	iGetFrameInProgress = 0;
	iEncodeInProgress = 0;
	
	TInt foundFileBased = 0; 
	TInt index = 0;	
	CVedVideoClip* tempclip =0;
	CVedMovieImp* tempmovie = (iMovie);
	TSize mRes = tempmovie->Resolution();
	
    if(iVideoClipNumber == 0)
    {
        while(foundFileBased == 0)
        {
            if(index >= iNumberOfVideoClips)
            {
                break;
            }
            else
            {
                tempclip = tempmovie->VideoClip(index);
                index++;
                if(tempclip->Info()->Class()==EVedVideoClipClassGenerated)
                {
                    foundFileBased = 0;  //You still need to search
                }
                else
                {
                    foundFileBased = 1;
                    
                    if (tempclip->Info()->FileHandle())
                    {
                        iClipFileName.Zero();
                        iClipFileHandle = tempclip->Info()->FileHandle();
                    }
                    else
                    {   
                        iClipFileHandle = NULL;                     
                        iClipFileName = tempclip->Info()->FileName();
                    }
                }
            }
        }
        tempmovie = 0;                /* make them zero again */
        tempclip = 0;
        if(foundFileBased == 0)     /* no file based clips in output movie */
        {
            iAllGeneratedClips =1;     /* to indicate all clips to be inserted are generated */
            iVideoParameters.iWidth = mRes.iWidth;
            iVideoParameters.iHeight = mRes.iHeight;
        }
        else
        {
            /* create an instance of the parser which will be used to set stream paramters from file based */
            if (iNumberOfVideoClips)
            {	                
                if (!iParser) 
                {
                    if (iClipFileHandle)
                        iParser = (CMP4Parser*) CMP4Parser::NewL(this, iClipFileHandle);
                    else
                        iParser = (CMP4Parser*) CMP4Parser::NewL(this, iClipFileName);
                }
                iParser->iFirstTimeClipParsing = ETrue;
                iState = EStateIdle;
                /* open file and parse header */                
                iDataFormat = EData3GP;     /* since file will be generated cant open with open stream so reset with standard */
                iMuxType = EMux3GP;    
                ParseHeaderL();
            }
        }
    }
    
    iState = EStatePreparing;
		
    if(iAllGeneratedClips == 1) /* all are generated clips => use H263 */
    {
		//iTranscoder->SetTargetVideoType(1);	/* set target video type to H263 */
		//not required as if all are generated even then output is based on engine output video type        
    }
    iMonitor->StartPreparing();
    if (!iVideoEncoder)
    {

#ifdef VIDEOEDITORENGINE_AVC_EDITING
        if (iOutputVideoType == EVideoAVCProfileBaseline)
        {
            VPASSERT(!iImageAvcEdit);
            iImageAvcEdit = CVedAVCEdit::NewL();
            
            iImageAvcEdit->SetOutputLevel( GetOutputAVCLevel() );
        }	
        
#endif
		iVideoEncoder = CVideoEncoder::NewL(iMonitor, iImageAvcEdit, iMovie->VideoCodecMimeType());
		
		iVideoEncoder->SetFrameSizeL(iMovie->Resolution());
		
		// Use the max frame rate since we don't want to change the frame rate
        TReal inputFrameRate = iMovie->MaximumFramerate();
        iVideoEncoder->SetInputFrameRate(inputFrameRate);
		
        if ( iMovie->VideoFrameRate() > 0 )
            iVideoEncoder->SetFrameRate( iMovie->VideoFrameRate() );
        if ( iMovie->VideoBitrate() > 0 ) // if there is request for restricted bitrate, use it
            iVideoEncoder->SetBitrate( iMovie->VideoBitrate() );
        else if ( iMovie->VideoStandardBitrate() > 0 ) // use the given standard bitrate
            iVideoEncoder->SetBitrate( iMovie->VideoStandardBitrate() );
        
        if( iMovie->RandomAccessRate() > 0.0 )
            iVideoEncoder->SetRandomAccessRate( iMovie->RandomAccessRate() );
        
        /* initialize encoder */        
		if (!IsActive())
		{
			SetActive();
			iStatus = KRequestPending;
		}        
        iVideoEncoder->InitializeL(iStatus);
		iEncoderInitPending = ETrue;
    }
    else
    {
		if((iAllGeneratedClips == 1) && (iVideoClipNumber == 0))
		{
			VPASSERT(iVideoEncoder->BeenStarted() == 0);
			//iVideoEncoder->Stop();
			delete iVideoEncoder;
			iVideoEncoder = 0;
			
#ifdef VIDEOEDITORENGINE_AVC_EDITING
			if (iOutputVideoType == EVideoAVCProfileBaseline)
            {
                VPASSERT(!iImageAvcEdit);
                iImageAvcEdit = CVedAVCEdit::NewL();
                
                iImageAvcEdit->SetOutputLevel( GetOutputAVCLevel() );
            }
#endif
			
			iVideoEncoder = CVideoEncoder::NewL(iMonitor, iImageAvcEdit, iMovie->VideoCodecMimeType());
			
			iVideoEncoder->SetFrameSizeL(iMovie->Resolution());
			
			TReal inputFrameRate = iMovie->MaximumFramerate();
            iVideoEncoder->SetInputFrameRate(inputFrameRate);
			
            if ( iMovie->VideoFrameRate() > 0 )
                iVideoEncoder->SetFrameRate( iMovie->VideoFrameRate() );
            if ( iMovie->VideoBitrate() > 0 ) // if there is request for restricted bitrate, use it
                iVideoEncoder->SetBitrate( iMovie->VideoBitrate() );
            else if ( iMovie->VideoStandardBitrate() > 0 ) // use the given standard bitrate
                iVideoEncoder->SetBitrate( iMovie->VideoStandardBitrate() );
            if( iMovie->RandomAccessRate() > 0.0 )
                iVideoEncoder->SetRandomAccessRate( iMovie->RandomAccessRate() );

			/* initialize encoder */			
			if (!IsActive())
			{
				SetActive();         
				iStatus = KRequestPending;
			}
			iVideoEncoder->InitializeL(iStatus);
			iEncoderInitPending = ETrue;			
		}
		else
		{
			// first frame has to be intra
			iVideoEncoder->SetRandomAccessPoint();
            iEncoderInitPending = ETrue;
            if (!IsActive())
            {
                SetActive();
                iStatus = KRequestPending;         
            }
            TRequestStatus *status = &iStatus;        
            User::RequestComplete(status, KErrNone);			
		}
    }
    
    
}


// -----------------------------------------------------------------------------
// CMovieProcessorImpl::ParseHeaderOnlyL
// Parses the header for a given clip
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CMovieProcessorImpl::ParseHeaderOnlyL(CParser::TStreamParameters& aStreamParams, 
                                           TFileName& aFileName, RFile* aFileHandle)
{
	
	if (!iParser)
    {        
		// create an instance of the parser 
		if (aFileHandle)
		    iParser = (CMP4Parser*) CMP4Parser::NewL(this, aFileHandle);
		else
		    iParser = (CMP4Parser*) CMP4Parser::NewL(this, aFileName);
	}
	iParser->ParseHeaderL(aStreamParams);
	
    // don't read the audio properties from input files, but from audio engine	
	CVedMovieImp* songmovie = (iMovie);
	CAudSong* songPointer = 0;
	
	if (songmovie)
	{
	    songPointer = songmovie->Song();    
	}		
	
	if (songPointer)
	{
	    TAudFileProperties prop = songPointer->OutputFileProperties();
	
    	aStreamParams.iHaveAudio = ETrue;
    	if (songPointer->ClipCount(KAllTrackIndices) == 0)
        {
    	    aStreamParams.iHaveAudio = EFalse;
    	}
    	    
    	aStreamParams.iAudioLength = I64INT(prop.iDuration.Int64())/1000;
    	aStreamParams.iAudioFormat = CParser::EAudioFormatNone;
    	if (prop.iAudioType == EAudAMR)
    	{
    	    aStreamParams.iAudioFormat = CParser::EAudioFormatAMR;
    	    aStreamParams.iAudioFramesInSample = 5;
    	    aStreamParams.iAudioTimeScale = 1000;
    	}
    	else if ( prop.iAudioType == EAudAAC_MPEG4 )
    	{
    	    aStreamParams.iAudioFormat = CParser::EAudioFormatAAC;
    	    aStreamParams.iAudioFramesInSample = 1;
    	    aStreamParams.iAudioTimeScale = prop.iSamplingRate;
    	}    	    
	}		

    // update output parameters. 
	UpdateStreamParameters(iParser->iStreamParameters, aStreamParams); 
	SetOutputNumberOfFrames(iParser->iOutputNumberOfFrames); 
}



// -----------------------------------------------------------------------------
// CMovieProcessorImpl::OpenStream
// Opens a clip for processing
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
TInt CMovieProcessorImpl::OpenStream(TFileName aFileName, RFile* aFileHandle, TDataFormat aDataFormat)
{
    // We can only streams in idle state
    if (iState != EStateIdle)
        return EInvalidProcessorState;

	TInt error = KErrNone;
	iDataFormat = aDataFormat;

    if (!aFileHandle)
    {
        
        RFs fs;
    	RFile file; 
            
    	if(aFileName.Length() == 0)
    		return KErrArgument;			
    	
    	// Open a file server session and open the file:
    	if ( (error = fs.Connect()) != KErrNone )
    		return error;
    	
    	if ( (error = file.Open(fs, aFileName, EFileShareReadersOnly | EFileRead)) != KErrNone )
        {        
            if ( (error = file.Open(fs, aFileName, EFileShareAny | EFileRead)) != KErrNone )
            {
               return error;
            }     
        }
    	
    	// set descriptor to read buffer
    	TPtr8 readDes(0,0);    
    	readDes.Set(iReadBuf, 0, KReadBufInitSize);
    	
    	// read data from the file 		
    	if ( (error = file.Read(readDes)) != KErrNone )
    		return error;
    	
    	if ( readDes.Length() < 8 )
    		return KErrGeneral;
    	
    	file.Close();
    	fs.Close();
    	
        // detect if format is 3GP, 5-8 == "ftyp"
        // This method is not 100 % proof, but good enough
        if ( (iReadBuf[4] == 0x66) && (iReadBuf[5] == 0x74) &&
            (iReadBuf[6] == 0x79) && (iReadBuf[7] == 0x70) )
        {
            iDataFormat = EData3GP;
            iMuxType = EMux3GP;        
        }
        else
            return KErrNotSupported;
    }
    
    // FIXME
    iDataFormat = EData3GP;
    iMuxType = EMux3GP;   

    // parse 3GP header
    CMP4Parser *parser = 0;
    if ( !iParser ) 
    {        
        if (iClipFileHandle)
        {
            TRAP(error, (parser = CMP4Parser::NewL(this, iClipFileHandle)) );
        }
        else
        {
            TRAP(error, (parser = CMP4Parser::NewL(this, iClipFileName)) );
        }
                
        if (error != KErrNone)
            return error;
        iParser = parser;
    }
    else
        parser = (CMP4Parser*)iParser;        

    TRAP(error, ParseHeaderL());

    if (error != KErrNone)
        return error;

    iState = EStateOpened;

    return KErrNone;

	   
}

// -----------------------------------------------------------------------------
// CMovieProcessorImpl::CloseStream
// Closes the processed stream from parser
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
TInt CMovieProcessorImpl::CloseStream()
{
 
	PRINT((_L("CMovieProcessorImpl::CloseStream() begin - iState = %d"), iState))

    if ( (iState != EStateOpened) && (iState != EStateProcessing) ) 
		return EInvalidProcessorState;
	
	TInt error=0;

	// delete parser
	if (iParser)
	{
		TRAP(error,
				{
			delete iParser;
			iParser=0;
				}
		);
		if (error != KErrNone)
			return error;
	}
	
	iClipFileName.Zero();
	iCurrentMovieName.Zero();
		
	// We are idle again
	iState = EStateIdle;

	PRINT((_L("CMovieProcessorImpl::CloseStream() end ")))
    
    return KErrNone;
}


// -----------------------------------------------------------------------------
// CMovieProcessorImpl::Prepare
// Prepares the processor for processing, opens demux & decoder
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
TInt CMovieProcessorImpl::Prepare()
{
    TInt error;	
	TUint videoBlockSize, videoBlocks;	

    // We can only prepare from preparing state
    if (iState != EStatePreparing)
        return EInvalidProcessorState;
	
	// Make sure we now know the stream format
	if (iDataFormat == EDataAutoDetect)
		return EUnsupportedFormat;	
	
    // Check whether the stream has audio, video or both, and whether it is
    // muxed
    switch (iDataFormat)
        {
        case EData3GP:
            // the video and audio flags are set when
            // the header is parsed.
            iIsMuxed = ETrue;
            break;
        default:
            User::Panic(_L("CMovieProcessorImpl"), EInvalidInternalState);
        }

	// If we have already played this stream since opening it, we'll have to
	// try to rewind
		
    // only 3gp file format supported => iIsMuxed always true    
    videoBlocks = KVideoQueueBlocks;
    videoBlockSize = KVideoQueueBlockSize;		
	
	// Initialize video
    VPASSERT((!iVideoQueue) && (!iVideoProcessor));
    if (iHaveVideo)
    {
        TRAP(error, InitVideoL(videoBlocks, videoBlockSize));
        if ( error != KErrNone )
            return error;
    }
	
	// Initialize demux
	VPASSERT(!iDemux);
    VPASSERT(iIsMuxed);
	TRAP(error, InitDemuxL());
    if ( error != KErrNone )
        return error;

	iState = EStateReadyToProcess;
    
	return KErrNone;
}

// -----------------------------------------------------------------------------
// CMovieProcessorImpl::InitVideoL
// Initializes the video decoder for processing
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//				
void CMovieProcessorImpl::InitVideoL(TUint aQueueBlocks, TUint aQueueBlockSize)
{
	// Create video input queue
	iVideoQueue = new (ELeave) CActiveQueue(aQueueBlocks, aQueueBlockSize);
	iVideoQueue->ConstructL();		
	
	if (iThumbnailInProgress && 
	    iParser->iStreamParameters.iVideoFormat == CParser::EVideoFormatAVCProfileBaseline)
    {
        if (!iAvcEdit)
        {            
            // create AVC editing instance
    	    iAvcEdit = CVedAVCEdit::NewL();
        }
    }
	
	// Create correct video decoder object

	VPASSERT(!iVideoProcessor);
	switch (iVideoType)
	{
	case EVideoH263Profile0Level10:
	case EVideoH263Profile0Level45:
	case EVideoMPEG4:
	case EVideoAVCProfileBaseline:
		// H.263 decoder handles both H.263+ and MPEG-4
		{

            iVideoProcessor = CVideoProcessor::NewL(iVideoQueue, 
                                                    &iVideoParameters, 
                                                    this, 
                                                    iMonitor,      
                                                    iAvcEdit,                                        
                                                    iThumbnailInProgress,
                                                    CActive::EPriorityStandard);
            
		}
		break;
		
	default:
		User::Leave(EUnsupportedFormat);
	}
}
				
				
// -----------------------------------------------------------------------------
// CMovieProcessorImpl::InitDemuxL
// Initializes the demultiplexer for processing
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//				
void CMovieProcessorImpl::InitDemuxL()
{	
	
	// Set video channel target queue
	TUint i;	

    // 3gp is the only supported file format
    VPASSERT(iMuxType == EMux3GP);    

	for ( i = 0; i < iNumDemuxChannels; i++ )
	{        
        if (iMP4Channels[i].iDataType == CMP4Demux::EDataVideo)
        {
            VPASSERT(iHaveVideo);
            iMP4Channels[i].iTargetQueue = iVideoQueue;
        }        
    }	
	
	VPASSERT(iParser);

    iDemux = CMP4Demux::NewL(NULL /* demuxQueue */, iNumDemuxChannels,
                             iMP4Channels, &iMP4Parameters,
                             iMonitor, (CMP4Parser*)iParser,
                             KDemuxPriority);
}

// -----------------------------------------------------------------------------
// CMovieProcessorImpl::DoStartProcessing
// Starts processing the movie
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//				
void CMovieProcessorImpl::DoStartProcessing()
{

    PRINT((_L("CMovieProcessorImpl::DoStartProcessing() begin")))

    if (iNumberOfVideoClips)
    {        
        VPASSERT(iDemux);
        VPASSERT(iVideoProcessor);
        VPASSERT(iState == EStateReadyToProcess);
        
        // start demuxing & decoding video

        iDemux->Start();
        iVideoProcessor->Start();
        if(!((iVideoClipNumber==0)&&((TVedVideoClipClass)iVideoClip->Info()->Class()==(TVedVideoClipClass)EVedVideoClipClassGenerated)))
            iMonitor->ProcessingStarted(iStartingProcessing);
        iStartingProcessing = EFalse;
        iState = EStateProcessing;
    }
    else
    {// audio-only case

        iState = EStateProcessing;
        iMonitor->ProcessingStarted(EFalse);
        
        TRAPD( error, iObserver->NotifyMovieProcessingStartedL(*iMovie) );
        if (error != KErrNone)
        {
            if (iMonitor)
                iMonitor->Error(error);
            return;
        }
        
        // process all audio clips
        ProcessAudioOnly();        
    }       

    PRINT((_L("CMovieProcessorImpl::DoStartProcessing() end")))

}


// -----------------------------------------------------------------------------
// CMovieProcessorImpl::ProcessAudioOnly
// Processes the movie in audio-only case
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//				
void CMovieProcessorImpl::ProcessAudioOnly()
{

    VPASSERT(iNumberOfAudioClips > 0);    

    // write audio frames to file & encode a black video frame
    TInt error;
    TRAP(error, ProcessAudioL());
    if (error != KErrNone)
    {
        iMonitor->Error(error);
        return;
    }

    //VPASSERT(iEncodePending);        

}

// -----------------------------------------------------------------------------
// CMovieProcessorImpl::DoCloseVideoL
// Closes & deletes the structures used in processing
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//				
void CMovieProcessorImpl::DoCloseVideoL()
{
	if ((iState == EStateProcessing) || (iState == EStateReadyToProcess)|| 
        (iState == EStatePreparing) )
	{
		PRINT((_L("CMovieProcessorImpl::DoCloseVideoL() - stopping")))
		User::LeaveIfError(Stop());
		iState = EStateOpened;
	}
	
	// If we are buffering or opening at the moment or clip is open then close it 
	if ( (iState == EStateOpened) || (iState == EStateReadyToProcess)) 
	{
		PRINT((_L("CMovieProcessorImpl::DoCloseVideoL() - closing stream")))
		User::LeaveIfError(CloseStream());
		iState = EStateIdle;
	}
}
				
				

// -----------------------------------------------------------------------------
// CMovieProcessorImpl::Stop
// Stops processing & closes modules used in processing
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//				
TInt CMovieProcessorImpl::Stop()
{
	TInt error = KErrNone; 
	
	// Check state
	if ( (iState != EStateProcessing) && (iState != EStateReadyToProcess) && (iState != EStatePreparing) )
		return EInvalidProcessorState;
	// We may also get here from the middle of a Prepare() attempt.

	PRINT((_L("CMovieProcessorImpl::Stop() begin")))
		
	// Destroy the playback objects to stop playback
	TRAP(error,
	{
        if (iDemux)
            delete iDemux; 
        iDemux = 0;
        
        if (iAudioProcessor)
            delete iAudioProcessor;
        iAudioProcessor = 0;
        
	});
	if (error != KErrNone)
		return error;
	
	
	if (iVideoEncoder)
	{
	    if (!iEncoderInitPending)
	    {
	        // Delete encoder. Don't delete now if encoding
	        // is not in progress at encoder and there are 
	        // active encoding flags. This means that encoder
	        // has completed encoding request, but this->RunL()
	        // hasn't been called yet. Encoder will be deleted in RunL()
	        
    	    if ( ( iVideoEncoder->IsEncodePending() == 1 ) ||
    	         ( (!iEncodePending && !iEncodeInProgress) ) )
    	    {
    	        PRINT((_L("CMovieProcessorImpl::Stop() - deleting encoder")));
    	        Cancel();
    	        iVideoEncoder->Stop();
    	        delete iVideoEncoder;
			    iVideoEncoder = 0;    	        
    	    }
	    }	    
	} 		  

	if (iVideoProcessor)
		delete iVideoProcessor;
	iVideoProcessor = 0;					
	
	if (iVideoQueue)
		delete iVideoQueue;
	iVideoQueue = 0;
		    
    if (!iThumbnailInProgress && iState == EStateProcessing)
    {   
        if (iMonitor)     
            iMonitor->ProcessingStopped();
    }

	iState = EStateOpened;
	
	PRINT((_L("CMovieProcessorImpl::Stop() end")))

	return KErrNone;
}


// -----------------------------------------------------------------------------
// CMovieProcessorImpl::Close
// Stops processing and closes all submodules except status monitor
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//	
TInt CMovieProcessorImpl::Close()
{
 
    // delete all objects except status monitor
    delete iComposer; 
    iComposer = 0;
    DeleteClipStructures();        
    TRAPD(error, DoCloseVideoL());
    if (error != KErrNone)
        return error;

    iState = EStateIdle;

    return KErrNone;

}


// -----------------------------------------------------------------------------
// CMovieProcessorImpl::RunL
// Called by the active scheduler when the video encoder initialization is done
// or an ending black frame has been encoded
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//	
void CMovieProcessorImpl::RunL()
{

    PRINT((_L("RunL begin, iEncoderInitPending %d, iEncodePending %d, iEncodeInProgress %d"), 
				iEncoderInitPending, iEncodePending, iEncodeInProgress ));
				
    PRINT((_L("CMovieProcessorImpl::RunL begin - iEncodeInProgress %d"), iEncodeInProgress));
		
    if (iAudioProcessingCompleted)
    {
        iAudioProcessingCompleted = EFalse;
        FinalizeVideoSequenceL();
        return;
    }
    	
	if (iNumberOfVideoClips)
	{
		// If we come here after a generated 3gp clip has been created, 
		// ival == EVedVideoClipClassGenerated && iImageClipCreated == 1		
		
		TVedVideoClipClass ival = (TVedVideoClipClass)iVideoClip->Info()->Class();
		if(ival == EVedVideoClipClassGenerated && iImageClipCreated == 0) // iImageClipCreated will work only for single imageset currently
		{	
			iEncoderInitPending = EFalse;  // is this correct ??
			DoImageSetProcessL(); // Call function which does image file creation
			return;
		}
	}    

	if (iEncoderInitPending)
    {		
		VPASSERT(!iEncodePending);

		PRINT((_L("CMovieProcessorImpl::RunL - encoder init complete")));
        // video encoder has been initialized => start processing
        
		iEncoderInitPending = EFalse;
		
		if (iProcessingCancelled)
		{
		    PRINT((_L("CMovieProcessorImpl::RunL - processing cancelled")));
		    if (iVideoEncoder)
		    {
		        iVideoEncoder->Stop();
		        delete iVideoEncoder;
		        iVideoEncoder = 0; 
		    }
		    PRINT((_L("CMovieProcessorImpl::RunL - calling cancelled callback")));
		    if (iMonitor)
		        iMonitor->ProcessingCancelled(); 

		    return;	    
		}
		
		if (iEncodingBlackFrames)
		{
		    FinalizeVideoSequenceL();
		    return;
		}
        
        if (iNumberOfVideoClips)
        {                                		    
				
#ifdef VIDEOEDITORENGINE_AVC_EDITING
            CVedVideoClipInfo* currentInfo = iVideoClip->Info();

            // Save SPS/PPS data from input file even if output type
            // is not AVC. This data is needed in doing blending transitions
            // from AVC input to H.263/MPEG-4 output. An optimisation
            // could be to check if such transitions are present in movie
            // and save only when necessary.

            if ( (currentInfo->Class() == EVedVideoClipClassFile && 
                  currentInfo->VideoType() == EVedVideoTypeAVCBaselineProfile) ||
                 (iOutputVideoType == EVideoAVCProfileBaseline && 
		          currentInfo->Class() == EVedVideoClipClassGenerated) )
		    {
		        // save SPS/PPS NAL units   
		        TInt size = iParser->GetDecoderSpecificInfoSize();
		        HBufC8* buf = (HBufC8*) HBufC8::NewLC(size);
		        TPtr8 ptr = buf->Des();
		        User::LeaveIfError( iParser->ReadAVCDecoderSpecificInfo(ptr) );
		        iAvcEdit->SaveAVCDecoderConfigurationRecordL(ptr, EFalse);
		        CleanupStack::PopAndDestroy();  // buf
		    }
#endif

            // go to the beginning of the I frame immediately preceding the start cut time 
            User::LeaveIfError(iParser->SeekOptimalIntraFrame(iStartCutTime, 0, ETrue));
            
            TInt currentFrameIndex = iOutputNumberOfFrames;
            
            // update parameters
            SetOutputNumberOfFrames(iParser->iOutputNumberOfFrames); 
            
            // fill in frame parameters
            FillFrameParametersL(currentFrameIndex);
            
            // open video decoder & demux
            User::LeaveIfError(Prepare());            
        }
				                        // if ( iVideoClipNumber > 0 || class != generated )
		if( iNumberOfVideoClips == 0 || !( (iVideoClipNumber==0) && ((TVedVideoClipClass)iVideoClip->Info()->Class()==(TVedVideoClipClass)EVedVideoClipClassGenerated)) )
			iMonitor->PrepareComplete();		

        // start processing
        DoStartProcessing();    
    }

    else if (iEncodePending)
    {
        // encoding complete
        iEncodePending = EFalse;
        
        TInt64 frameDuration;       // in ticks
        if ( iLeftOverDuration >= 1000 )
        {
            iLeftOverDuration -= 1000;
            frameDuration = iOutputVideoTimeScale; // One second in ticks
        }
        else
        {
            iLeftOverDuration = 0;
            frameDuration = TInt64( TReal(iOutputVideoTimeScale)/1000.0 * I64REAL(iLeftOverDuration) + 0.5 );
        }

        if (iStatus == KErrNone)
        {            
            // Audio-only or audio longer than video
            // => black frame is encoded at the end
            
            PRINT((_L("CMovieProcessorImpl::RunL - encoding complete")));            
            
            // fetch the bitstream & write it to output file, release bitstream buffer        
            TBool isKeyFrame = 0;
            TPtrC8 buf(iVideoEncoder->GetBufferL(isKeyFrame));

            TReal tsInTicks = I64REAL(iTimeStamp.Int64() / TInt64(1000)) * TReal(iOutputVideoTimeScale) / 1000.0;
            
            User::LeaveIfError( WriteVideoFrameToFile((TDesC8&)buf, TInt64(tsInTicks + 0.5), I64INT(frameDuration), 
                                                      isKeyFrame, ETrue, EFalse, ETrue ) );        
            
            iVideoEncoder->ReturnBuffer();                        
            
            // do not reset flag until here so that last frame of last clip gets the correct 
            // duration in case slow motion is used
            iApplySlowMotion = EFalse;
        }
#ifdef _DEBUG
        else
        {            
            PRINT((_L("CMovieProcessorImpl::RunL - encoding failed")));
        }
#endif
        
        iTimeStamp = TTimeIntervalMicroSeconds(iTimeStamp.Int64() + TInt64(1000000));

		if (iLeftOverDuration > 0)
		{
			// encode another frame
			VPASSERT(iEncoderBuffer);
        	TSize tmpSize = iMovie->Resolution();
            TUint yLength = tmpSize.iWidth * tmpSize.iHeight;
			TUint uvLength = yLength >> 1;
			TUint yuvLength = yLength + uvLength;
			TPtr8 yuvPtr(iEncoderBuffer, yuvLength, yuvLength);
			
            if (!IsActive())
            {
                SetActive();
                iStatus = KRequestPending;
            }
			
			iVideoEncoder->EncodeFrameL(yuvPtr, iStatus, iTimeStamp);
            iEncodePending = ETrue;
			
		}
		else
		{		
		    // movie complete, close everything
		    
            TInt error = FinalizeVideoWrite();
            if (error != KErrNone)
			{
                User::Leave(KErrGeneral);				
			}

			error = iComposer->Close();  // creates the output file
			if (error != KErrNone)
			{
                User::Leave(KErrGeneral);				
			}			
			
			// delete all objects except status monitor
			delete iComposer; 
			iComposer = 0;						
			
			if(iImageComposer) 
		    {
    			delete iImageComposer;
    			iImageComposer=0;
		    }
		    
		    if (iImageAvcEdit)
        	{
        	    delete iImageAvcEdit;
        	    iImageAvcEdit = 0;
        	}
		 
			DeleteClipStructures();
			
			DoCloseVideoL();
			
			VPASSERT(!iEncoderInitPending);
			
			PRINT((_L("CMovieProcessorImpl::RunL - calling completed callback")));
			iMonitor->ProcessingComplete();
            iState = EStateIdle;
		}
		
    }
}


// -----------------------------------------------------------------------------
// CMovieProcessorImpl::RunError
// Called by the AO framework when RunL method has leaved
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//	
TInt CMovieProcessorImpl::RunError(TInt aError)
{

    if ( iCurClipFileName.Length() )
	{
        iCurClip.Close();
        iFs.Delete( iCurClipFileName );
        iCurClipFileName.Zero();
        iCurClipDurationList.Reset();
        iCurClipTimeStampList.Reset();
    }

    iMonitor->Error(aError);

    return KErrNone;
}


// -----------------------------------------------------------------------------
// CMovieProcessorImpl::DoCancel
// Cancels any pending asynchronous requests
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CMovieProcessorImpl::DoCancel()
{

	PRINT((_L("CMovieProcessorImpl::DoCancel() begin")))

    // Cancel our internal request
    if ( iStatus == KRequestPending )
    {
		PRINT((_L("CMovieProcessorImpl::DoCancel() cancel request")))
        TRequestStatus *status = &iStatus;
        User::RequestComplete(status, KErrCancel);
    }

	PRINT((_L("CMovieProcessorImpl::DoCancel() end")))
}


// -----------------------------------------------------------------------------
// CMovieProcessorImpl::SetHeaderDefaults
// Sets appropriate default values for processing parameters
// in audio-only case
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CMovieProcessorImpl::SetHeaderDefaults()
{

    // set suitable default values
	iHaveVideo = ETrue;
	iVideoType = EVideoH263Profile0Level10;
	iHaveAudio = ETrue;
	iAudioType = EAudioAMR;
	iNumDemuxChannels = 0;
		
    // resolution from movie
	TSize tmpSize = iMovie->Resolution();
	iVideoParameters.iWidth = tmpSize.iWidth;
	iVideoParameters.iHeight = tmpSize.iHeight;
	iVideoParameters.iIntraFrequency = 0;
	iVideoParameters.iNumScalabilityLayers = 0;
	iVideoParameters.iReferencePicturesNeeded = 0;
    // picture period in nanoseconds
    iVideoParameters.iPicturePeriodNsec = TInt64(33366667);

    // output time scales
    iOutputVideoTimeScale = KVideoTimeScale;
    iOutputAudioTimeScale = KAMRAudioTimeScale;

    iStreamLength = 0;
    iStreamSize = 0;
    iStreamBitrate = 10000;

}


// -----------------------------------------------------------------------------
// CMovieProcessorImpl::ParseHeaderL
// Parses the clip header & sets internal variables accordingly
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CMovieProcessorImpl::ParseHeaderL()
{
	TInt i;
	
	VPASSERT(iParser);
	
	if ( iMuxType != EMux3GP )
        User::Leave(EUnsupportedFormat);
	
	CParser::TStreamParameters streamParams;
	
	// parse
	iParser->ParseHeaderL(streamParams);
	
    // don't read the audio properties from input files, but from audio engine	
	CVedMovieImp* songmovie = (iMovie);
	CAudSong* songPointer = 0;
	
	if (songmovie)
	{
	    songPointer = songmovie->Song();    
	}
			
	if (songPointer)
	{
	    
	    TAudFileProperties prop = songPointer->OutputFileProperties();
	
	    streamParams.iHaveAudio = ETrue;
    	if (songPointer->ClipCount(KAllTrackIndices) == 0)
    	{
    	    streamParams.iHaveAudio = EFalse;
    	}
    	    
    	streamParams.iAudioLength = I64INT(prop.iDuration.Int64())/1000;
    	streamParams.iAudioFormat = CParser::EAudioFormatNone;
    	if (prop.iAudioType == EAudAMR)
    	{
    	    streamParams.iAudioFormat = CParser::EAudioFormatAMR;
    	    streamParams.iAudioFramesInSample = 5;
    	    streamParams.iAudioTimeScale = 1000;
    	}
    	else if ( prop.iAudioType == EAudAAC_MPEG4 )
    	{
    	    streamParams.iAudioFormat = CParser::EAudioFormatAAC;
    	    streamParams.iAudioFramesInSample = 1;
    	    streamParams.iAudioTimeScale = prop.iSamplingRate;
    	}    	    
	}	

	// copy input stream info into parser
	UpdateStreamParameters(iParser->iStreamParameters, streamParams); 
	
	// copy parameters
	iHaveVideo = streamParams.iHaveVideo;
	iVideoType = (TVideoType)streamParams.iVideoFormat;
	iHaveAudio = streamParams.iHaveAudio;
	iAudioType = (TAudioType)streamParams.iAudioFormat;
	iNumDemuxChannels = streamParams.iNumDemuxChannels;
	iCanSeek = streamParams.iCanSeek;
	iVideoParameters.iWidth = streamParams.iVideoWidth;
	iVideoParameters.iHeight = streamParams.iVideoHeight;
	iVideoParameters.iIntraFrequency = streamParams.iVideoIntraFrequency;
	iVideoParameters.iNumScalabilityLayers = streamParams.iNumScalabilityLayers;
	iVideoParameters.iReferencePicturesNeeded = streamParams.iReferencePicturesNeeded;
	iVideoParameters.iPicturePeriodNsec = streamParams.iVideoPicturePeriodNsec;

	for (i = 0; i < (TInt)streamParams.iNumScalabilityLayers; i++)
		iVideoParameters.iLayerFrameRates[i] = streamParams.iLayerFrameRates[i];

    
	// assign time scale values
	if((iVideoClipNumber==0) && (iOutputVideoTimeSet==0))
	{
		iOutputVideoTimeSet = 1;	
		iOutputVideoTimeScale = KVideoTimeScale;
	}	
   	
	// change the start and end times of the video clip from msec to ticks
	iVideoClipParameters[iVideoClipNumber].iStartTime = TInt64( TReal(iOutputVideoTimeScale)/1000.0 *
		I64REAL(iVideoClipParameters[iVideoClipNumber].iStartTime) ); 

	iVideoClipParameters[iVideoClipNumber].iEndTime = TInt64( TReal(iOutputVideoTimeScale)/1000.0 *
		I64REAL(iVideoClipParameters[iVideoClipNumber].iEndTime) );

    iVideoParameters.iTiming = CVideoProcessor::ETimeStamp;
    iMP4Parameters.iPicturePeriodMs = I64INT( (iVideoParameters.iPicturePeriodNsec / TInt64(1000000)) );
    iMP4Parameters.iAudioFramesInSample = streamParams.iAudioFramesInSample;		
	
	
	if ( iHaveVideo ) 
    {	
		iMP4Channels[0].iDataType = CMP4Demux::EDataVideo;
	}

    if ( iHaveAudio ) {

        iNumDemuxChannels = 1;

        // NOTE: audio is processed 'off-line' after a video
        // clip has been processed.

		//iMP4Channels[i].iDataType = CMP4Demux::EDataAudio;
	}
	
	iStreamLength = streamParams.iStreamLength;
	iStreamBitrate = streamParams.iStreamBitrate;
	iStreamSize = streamParams.iStreamSize;		

	// Ensure that the video isn't too large
	if (!iThumbnailInProgress)
	{
    	if ( (iVideoParameters.iWidth > KVedMaxVideoWidth) ||
    		(iVideoParameters.iHeight > KVedMaxVideoHeight) )
    		User::Leave(EVideoTooLarge);
	}

}

// -----------------------------------------------------------------------------
// CMovieProcessorImpl::UpdateStreamParameters
// Copies stream parameters to destination structure
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CMovieProcessorImpl::UpdateStreamParameters(CParser::TStreamParameters& aDestParameters,  
                                                 CParser::TStreamParameters& aSrcParameters)
{
		TInt i;
		aDestParameters.iHaveVideo = aSrcParameters.iHaveVideo; 
		aDestParameters.iHaveAudio = aSrcParameters.iHaveAudio; 
		aDestParameters.iNumDemuxChannels = aSrcParameters.iNumDemuxChannels; 
		aDestParameters.iFileFormat = aSrcParameters.iFileFormat;
		aDestParameters.iVideoFormat = aSrcParameters.iVideoFormat; 
		aDestParameters.iAudioFormat = aSrcParameters.iAudioFormat; 
		aDestParameters.iAudioFramesInSample = aSrcParameters.iAudioFramesInSample; 
		aDestParameters.iVideoWidth = aSrcParameters.iVideoWidth; 
		aDestParameters.iVideoHeight = aSrcParameters.iVideoHeight; 
		aDestParameters.iVideoPicturePeriodNsec = aSrcParameters.iVideoPicturePeriodNsec; 
		aDestParameters.iVideoIntraFrequency = aSrcParameters.iVideoIntraFrequency; 
		aDestParameters.iStreamLength = aSrcParameters.iStreamLength; 
		aDestParameters.iVideoLength = aSrcParameters.iVideoLength; 
		aDestParameters.iAudioLength = aSrcParameters.iAudioLength; 
		aDestParameters.iCanSeek = aSrcParameters.iCanSeek; 
		aDestParameters.iStreamSize = aSrcParameters.iStreamSize; 
		aDestParameters.iStreamBitrate = aSrcParameters.iStreamBitrate; 
		aDestParameters.iMaxPacketSize = aSrcParameters.iMaxPacketSize; 
		aDestParameters.iLogicalChannelNumberVideo = aSrcParameters.iLogicalChannelNumberVideo; 
		aDestParameters.iLogicalChannelNumberAudio = aSrcParameters.iLogicalChannelNumberAudio; 
		aDestParameters.iReferencePicturesNeeded = aSrcParameters.iReferencePicturesNeeded; 
		aDestParameters.iNumScalabilityLayers = aSrcParameters.iNumScalabilityLayers; 
		for(i=0; i<(TInt)aSrcParameters.iNumScalabilityLayers; i++)
			aDestParameters.iLayerFrameRates[i] = aSrcParameters.iLayerFrameRates[i];

		aDestParameters.iFrameRate = aSrcParameters.iFrameRate; 
		aDestParameters.iVideoTimeScale = aSrcParameters.iVideoTimeScale; 
		aDestParameters.iAudioTimeScale = aSrcParameters.iAudioTimeScale; 

}



// -----------------------------------------------------------------------------
// CMovieProcessorImpl::FinalizeVideoClip
// Finalizes video clip once all its frames are processed
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CMovieProcessorImpl::FinalizeVideoClip()
{	

	PRINT((_L("CMovieProcessorImpl::FinalizeVideoClip() begin")))

    TInt error = KErrNone;	

	TInt64 endPosition = TInt64(iCurrentVideoTimeInTicks + 0.5);

    if ( TransitionDuration() )
    {
		endPosition += TInt64( (TReal)TransitionDuration() * (TReal)iOutputVideoTimeScale / (TReal)iParser->iStreamParameters.iVideoTimeScale + 0.5);
    }

	// convert time from ticks to millisec.
	endPosition = GetVideoTimeInMsFromTicks( endPosition, ETrue );

	CVedMovieImp* movie = (iMovie);

	iLeftOverDuration = movie->Duration().Int64()/1000 - 
		movie->VideoClip(iNumberOfVideoClips-1)->EndTime().Int64()/1000;

    if (error != KErrNone)
    {
        iMonitor->Error(error);
        return;
    }
	
	// if last video clip in movie
	if(iVideoClipNumber == iNumberOfVideoClips-1 || iMovieSizeLimitExceeded)
	{
		iAllVideoProcessed = ETrue;
		if ( iFsConnected )
        {
            TRAP(error, CloseTransitionInfoL());
            if (error != KErrNone)
            {	
                iMonitor->Error(error);
                return;
            }
            iFs.Close();
            iFsConnected = EFalse;
        }

        // process all audio
        TRAP(error, ProcessAudioL());
        if (error != KErrNone)
        {
            iMonitor->Error(error);
            return;
        }
	}
	else // process the next clip
	{        

		if (iCurClipFileName.Length() )
        {
            iCurClip.Close();
            error = iFs.Delete( iCurClipFileName );
            if (error != KErrNone)
            {	
                iMonitor->Error(error);
                return;
            }
            iCurClipFileName.Zero();
            iCurClipDurationList.Reset();
            iCurClipTimeStampList.Reset();
            iTimeStampListScaled = EFalse;            
        }
		
        if ( iNextClipFileName.Length() )
        {
            iNextClip.Close();
        }

		// close the video 
        TRAP(error, DoCloseVideoL());
        if (error != KErrNone)
        {
            iMonitor->Error(error);

			if ( iNextClipFileName.Length() )
            {
                error = iFs.Delete( iNextClipFileName );
                if (error != KErrNone)
                {	
                    iMonitor->Error(error);
                    return;
                }
                iNextClipFileName.Zero();
                iNextClipDurationList.Reset();
                iNextClipTimeStampList.Reset();
            }
            return;
        }
        iMonitor->Closed();	

		if ( iFsConnected )
        {
            CFileMan *fileMan = 0;
			TRAP(error, fileMan = CFileMan::NewL( iFs ));
            if (error != KErrNone)
            {
                iMonitor->Error(error);
                return;
            }
			TParse filepath;
			filepath.Set( iOutputMovieFileName, NULL, NULL );
			TFileName filesToDelete = filepath.DriveAndPath();
			filesToDelete.Append( _L( "Im_nokia_vpi.tmp" ) );
			fileMan->Delete( filesToDelete,0 );
			delete fileMan;	
        }

		// copy from current to next
        iCurClipFileName = iNextClipFileName;
        iNextClipFileName.Zero();
        TInt duration;
        TInt64 timestamp;
        while ( iNextClipDurationList.Count() )
        {
            duration = iNextClipDurationList[0];
            iNextClipDurationList.Remove( 0 );
            iCurClipDurationList.Append( duration );
            timestamp = iNextClipTimeStampList[0];
            iNextClipTimeStampList.Remove( 0 );
            iCurClipTimeStampList.Append( timestamp );
        }

        iNextClipDurationList.Reset();
        iNextClipTimeStampList.Reset();
        iCurClipIndex = 0;

        if ( iCurClipFileName.Length() )
        {
            if ( iCurClip.Open(iFs, iCurClipFileName, EFileShareReadersOnly | EFileStream | EFileRead) != KErrNone )
            {
                if ( iCurClip.Open(iFs, iCurClipFileName, EFileShareAny | EFileStream | EFileRead) != KErrNone )
                {
                    iCurClip.Close();
                    iCurClipFileName.Zero();
                    iCurClipDurationList.Reset();
                    iCurClipTimeStampList.Reset();
                }
            }
        }

        VPASSERT(!iEncoderInitPending);
        
		// go to next clip
		iVideoClipNumber++;
		iVideoClip = movie->VideoClip(iVideoClipNumber);  

        if(iVideoClip->Info()->Class() == EVedVideoClipClassGenerated)
        {
            TRAP(error, TemporaryInitializeGeneratedClipL());
        }
        else
        {
            TRAP(error, InitializeClipL());
        }
		if (error != KErrNone)
		{
			if ( iCurClipFileName.Length() )
			{
				iCurClip.Close();
				TInt fsError = iFs.Delete( iCurClipFileName );
                if (fsError != KErrNone)
                {	
                    iMonitor->Error(fsError);
                    return;
                }
				iCurClipFileName.Zero();
				iCurClipDurationList.Reset();
				iCurClipTimeStampList.Reset();
			}
			iMonitor->Error(error);
			return;
		}
		
	}
	PRINT((_L("CMovieProcessorImpl::FinalizeVideoClip() end")))
}


// -----------------------------------------------------------------------------
// CMovieProcessorImpl::ProcessAudioL
// Starts audio processing
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CMovieProcessorImpl::ProcessAudioL()
{
	
	VPASSERT(iAudioProcessor == 0);	
	
	CVedMovieImp* songmovie = (iMovie); 	
	CAudSong* songPointer = songmovie->Song();
	
	// always read audio decoder specific info from audio engine
	TAudType audioType = songPointer->OutputFileProperties().iAudioType;
      	
    if( audioType == EAudAAC_MPEG4 )
    {        
	    HBufC8* audioinfo = 0;
	    songPointer->GetMP4DecoderSpecificInfoLC(audioinfo,64);  	
    	
	    if (audioinfo != 0)
	    {	        
		    CMP4Composer* composeInfo =  (CMP4Composer*)iComposer;
			User::LeaveIfError(composeInfo->WriteAudioSpecificInfo(audioinfo));
    	    CleanupStack::Pop();
    	    delete audioinfo;
    	    audioinfo = 0;
	    }
	    
    }
    if( (iMovieSizeLimit > 0)&&(iMovieSizeLimitExceeded) )
	{
	    songPointer->SetDuration(iEndCutTime.Int64()*1000);
	}
	
	// create audioprocessor 	
	iAudioProcessor = CAudioProcessor::NewL(this, songPointer);		                       
			
	// start processing
	iAudioProcessor->StartL();
	 
	
}

// -----------------------------------------------------------------------------
// CMovieProcessorImpl::AudioProcessingComplete
// Called by audio processor when audio processing has been completed
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CMovieProcessorImpl::AudioProcessingComplete(TInt aError)
{

    if (aError != KErrNone)
    {    	    
        iMonitor->Error(aError);
        return;
    }
    
    if (iProcessingCancelled)
        return;
    
    TInt error = FinalizeAudioWrite();
    if (error != KErrNone)
    {
        iMonitor->Error(error);
        return;    	
    }
    
    iAudioProcessingCompleted = ETrue;
    
    // since this is run in audio processor's RunL, the audio processor 
    // cannot be deleted here. signal the AO to finish in this->RunL()
    
    VPASSERT(!IsActive());
    
    SetActive(); 
    iStatus = KRequestPending;
    TRequestStatus *status = &iStatus;
    User::RequestComplete(status, KErrNone);
                        
    
    	
}

// -----------------------------------------------------------------------------
// CMovieProcessorImpl::FinalizeVideoSequence
// Finalizes the movie once all clips have been processed
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CMovieProcessorImpl::FinalizeVideoSequenceL()
{    

    CVedMovieImp* movie = (iMovie);

    // calculate video left-over duration in case audio runs longer than video
    iLeftOverDuration = 0;
    if (iNumberOfVideoClips > 0)
    {
        iLeftOverDuration = movie->Duration().Int64()/1000 - 
            movie->VideoClip(iNumberOfVideoClips-1)->EndTime().Int64()/1000;
    }
	else 
		iLeftOverDuration = movie->Duration().Int64()/1000;	

	if ( iLeftOverDuration > 0)
	{   
		VPASSERT( iNumberOfAudioClips!=0 );
        // encode black frames
        
    	if(iVideoProcessor)
        {
    		delete iVideoProcessor;	
            iVideoProcessor = 0;	
        }
        
        if (!iVideoEncoder)
        {
            // create and initialise encoder
        
            iVideoEncoder = CVideoEncoder::NewL(iMonitor, iAvcEdit, iMovie->VideoCodecMimeType());
			
			iVideoEncoder->SetFrameSizeL(iMovie->Resolution());
			
            if ( iMovie->VideoFrameRate() > 0 )
                iVideoEncoder->SetFrameRate( iMovie->VideoFrameRate() );
            if ( iMovie->VideoBitrate() > 0 ) // if there is request for restricted bitrate, use it
                iVideoEncoder->SetBitrate( iMovie->VideoBitrate() );
            else if ( iMovie->VideoStandardBitrate() > 0 ) // use the given standard bitrate
                iVideoEncoder->SetBitrate( iMovie->VideoStandardBitrate() );
            
            // use input framerate of 1 fps
            iVideoEncoder->SetInputFrameRate(1.0);
            
            if( iMovie->RandomAccessRate() > 0.0 )
                iVideoEncoder->SetRandomAccessRate( iMovie->RandomAccessRate() );

            // async.			
			if (!IsActive())
			{
				SetActive();
				iStatus = KRequestPending;
			}			
			iVideoEncoder->InitializeL(iStatus);			
			
			iEncoderInitPending = ETrue;
			iEncodingBlackFrames = ETrue;
			return;
			
        }
    	TSize tmpSize = iMovie->Resolution();
        TUint yLength = tmpSize.iWidth * tmpSize.iHeight;
		TUint uvLength = yLength >> 1;
		TUint yuvLength = yLength + uvLength;

        if ( iEncoderBuffer == 0 )
        {
            // allocate memory for encoder input YUV frame            
            iEncoderBuffer = (TUint8*)User::AllocL(yuvLength);                  
        }        

        // fill buffer with 'black' data
		// Y
        TPtr8 yuvPtr(0,0);
		TInt data=5;  // don't use zero - real player doesn't show all-zero frames
		yuvPtr.Set(iEncoderBuffer, yLength, yLength); 
		yuvPtr.Fill((TChar)data, yLength);

		// U,V
		data=128;
		yuvPtr.Set(iEncoderBuffer + yLength, uvLength, uvLength); 
		yuvPtr.Fill((TChar)data, uvLength);

        yuvPtr.Set(iEncoderBuffer, yuvLength, yuvLength);
        
        if (iNumberOfVideoClips == 0)
            iTimeStamp = 0;
        else
            iTimeStamp = movie->VideoClip(iNumberOfVideoClips-1)->CutOutTime();            

        if (!IsActive())
        {
            SetActive();
            iStatus = KRequestPending;
        }
        
        iVideoEncoder->EncodeFrameL(yuvPtr, iStatus, iTimeStamp);        
        iEncodePending = ETrue;

        return;
		
	}
	
	// movie complete, close everything
		
	User::LeaveIfError(FinalizeVideoWrite());
	
    // delete all objects except status monitor
	if (iComposer)
	{   
	    User::LeaveIfError(iComposer->Close());
		delete iComposer; 
		iComposer = 0;
	}

    DoCloseVideoL();    
    			
	DeleteClipStructures();
	
	PRINT((_L("CMovieProcessorImpl::FinalizeVideoSequence() - calling completed callback")))
	
	iMonitor->ProcessingComplete();
	iState = EStateIdle;
	
}

// -----------------------------------------------------------------------------
// CMovieProcessorImpl::CurrentMetadataSize
// Get current metadata size
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
TUint CMovieProcessorImpl::CurrentMetadataSize()
{
	TBool haveAudio;
    TBool haveVideo;
    TUint metadatasize = 0;

    haveAudio = EFalse;
    haveVideo = EFalse;

	if ( GetOutputVideoType() == EVedVideoTypeH263Profile0Level10 || 
		 GetOutputVideoType() == EVedVideoTypeH263Profile0Level45 )
	{
		haveVideo = ETrue;
        metadatasize += 574; // Constant size H.263 metadata
        metadatasize += (iVideoFrameNumber * 16 + iVideoIntraFrameNumber * 4); // Content dependent H.263 metadata
	}

	if ( GetOutputVideoType() == EVedVideoTypeMPEG4SimpleProfile )
	{
		haveVideo = ETrue;
        metadatasize += 596; // Constant size MPEG-4 video metadata
        metadatasize += (iVideoFrameNumber * 16 + iVideoIntraFrameNumber * 4); // Content dependent MPEG-4 video metadata
	}

	if ( GetOutputAudioType() == EVedAudioTypeAMR ) // AMR-NB
	{
        haveAudio = ETrue;
        metadatasize += 514; // Constant size AMR metadata
        metadatasize += ((iAudioFrameNumber + KVedAudioFramesInSample - 1) / KVedAudioFramesInSample) * 8;
	}

	if ( GetOutputAudioType() == EVedAudioTypeAAC_LC ) // MPEG-4 AAC-LC
	{
        haveAudio = ETrue;
        metadatasize += 514; // Constant size metadata
        metadatasize += (iAudioFrameNumber * 8);
	}

	if (haveAudio && haveVideo)
        metadatasize -= 116; // There is only one moov and mvhd in a file

	return metadatasize;
		
}

// -----------------------------------------------------------------------------
// CMovieProcessorImpl::BufferAMRFrames
// Collects output audio frames to a buffer and writes them
// to the output 3gp file when a whole audio sample is available
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
TInt CMovieProcessorImpl::BufferAMRFrames(const TDesC8& aBuf, TInt aNumFrames,TInt aDuration)

{

	VPASSERT(iOutAudioBuffer); 
	TPtr8 outAudioPtr(iOutAudioBuffer->Des());
	TInt error = KErrNone;
	
	if((outAudioPtr.Length() + aBuf.Length()) > outAudioPtr.MaxLength()) 
	{
		// extend buffer size
		
		// New size is 3/2ths of the old size, rounded up to the next
		// full kilobyte       
		TUint newSize = (3 * outAudioPtr.MaxLength()) / 2;
		newSize = (newSize + 1023) & (~1023);
		TRAP(error, (iOutAudioBuffer = iOutAudioBuffer->ReAllocL(newSize)) );
		
		if (error != KErrNone)
			return error;
		
		PRINT((_L("CMovieProcessorImpl::BufferAMRFrames() - extended buffer to %d bytes"), newSize));
			
		outAudioPtr.Set(iOutAudioBuffer->Des());
	}
	outAudioPtr.Append(aBuf);
    iTotalDurationInSample += aDuration;
	iAudioFramesInBuffer += aNumFrames;

	return KErrNone;
}


// -----------------------------------------------------------------------------
// CMovieProcessorImpl::WriteAMRSamplesToFile
// Write buffered AMR sample(s) to composer
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
TInt CMovieProcessorImpl::WriteAMRSamplesToFile()
{

	VPASSERT(iOutAudioBuffer); 
	TPtr8 outAudioPtr(iOutAudioBuffer->Des());

	VPASSERT(TUint(iAudioFramesInSample) == KVedAudioFramesInSample);

	while ( iAudioFramesInBuffer >= iAudioFramesInSample )
    {
        // write one audio sample to file

//        TInt frameSamples = 160;
//        TInt samplingRate = KVedAudioSamplingRate8k;
        TInt audioSampleDurationInTicks = 0;
        TPtrC8 writeDes;
        TInt sampleSize = 0;
//        TInt frameNum = 0;	
//        TInt frameSize = 0;
        TUint8* frameBuffer = (TUint8*)outAudioPtr.Ptr();
		TInt error = KErrNone;

        // Gets Size of buffer of current frames
        sampleSize = outAudioPtr.Length();
                    
        // if frame is sampled at a different rate 
        audioSampleDurationInTicks = GetAudioTimeInTicksFromMs(iTotalDurationInSample/1000);
                
        // set descriptor to sample        
        writeDes.Set( outAudioPtr.Left(sampleSize) );		

        // compose audio to output 3gp file
        error = iComposer->WriteFrames((TDesC8&)writeDes, sampleSize, audioSampleDurationInTicks, 0 /*iAudioKeyFrame*/, 
            iAudioFramesInSample, CMP4Composer::EFrameTypeAudio);        
            
        // Resetting duration to zero
		iTotalDurationInSample = 0;

        if (error != KErrNone)
            return error;

        if ( outAudioPtr.Length() > sampleSize )
        {
            // copy rest of the data to beginning of buffer
            frameBuffer = (TUint8*)outAudioPtr.Ptr();
            TInt len = outAudioPtr.Length() - sampleSize;
            Mem::Copy(frameBuffer, frameBuffer + sampleSize, len);
            outAudioPtr.SetLength(len);
        }
        else 
        {
            VPASSERT(iAudioFramesInBuffer == iAudioFramesInSample);
            outAudioPtr.SetLength(0);
        }
                     
        iAudioFramesInBuffer -= iAudioFramesInSample;
		iAudioFrameNumber += iAudioFramesInSample;
    }
	return KErrNone;

}

TInt CMovieProcessorImpl::WriteAllAudioFrames(TDesC8& aBuf, TInt aDuration)
{

    if (iDiskFull)
	    return KErrDiskFull;

    // check available disk space 	 

	TInt error;
	TInt64 freeSpace = 0;
	// get free space on disk
	TRAP(error, freeSpace = iComposer->DriveFreeSpaceL());
	if (error != KErrNone)
			return error;

	// subtract metadata length from free space
    freeSpace -= TInt64(CurrentMetadataSize());

	if (freeSpace < TInt64(KDiskSafetyLimit))
	{
			iDiskFull = ETrue;
			return KErrDiskFull;
	}


	if ( GetOutputAudioType() == EVedAudioTypeAMR )
	{ 	
     	// write frame by frame until duration >= duration for this clip
     	
     	// NOTE: clip duration not checked!!!
     	
		TPtr8 ptr(0,0);
		TUint8* buf = (TUint8*)aBuf.Ptr();
		TInt frameSize = aBuf.Length();

		ptr.Set(buf, frameSize, frameSize);
		error = BufferAMRFrames(ptr, 1,aDuration);
		if ( error != KErrNone )
			return error;
		iTotalAudioTimeWrittenMs += (aDuration/1000); //20;
		buf += frameSize;
	
	}
	else
	{
		// write buffer directly to file (1 frame per sample in AAC)	
		
		error = KErrNone;
//		TInt error = KErrNone;
		TInt sampleSize = aBuf.Length();
//		const TInt framesInSample = 1;
		TInt audioSampleDurationInTicks = GetAudioTimeInTicksFromMs(aDuration/1000); 
		
		iTotalAudioTimeWrittenMs += (aDuration/1000);
						
		VPASSERT(iAudioFramesInSample == 1);
		error = iComposer->WriteFrames(aBuf, sampleSize, audioSampleDurationInTicks, 0 /*iAudioKeyFrame*/, 
			iAudioFramesInSample, CMP4Composer::EFrameTypeAudio);
	} 	

	if ( GetOutputAudioType() == EVedAudioTypeAMR )
	{
		error = WriteAMRSamplesToFile();
		if ( error != KErrNone )
			return error;
	}

	if ( (iTotalAudioTimeWrittenMs % 200) == 0 )
		IncProgressBar();

	return KErrNone;
	
}


// -----------------------------------------------------------------------------
// CMovieProcessorImpl::FinalizeAudioWrite
// Write the remaining audio frames at the end of processing
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
TInt CMovieProcessorImpl::FinalizeAudioWrite()
{

    VPASSERT(iAudioFramesInBuffer < iAudioFramesInSample);

    if (iAudioFramesInBuffer == 0)
        return KErrNone;

	if(iOutputAudioType == EAudioAAC)
	{
		// There should not be any frames in buffer !!
		VPASSERT(0);
	}

    TInt error = KErrNone;
    TInt sampleSize = 0;
//    TInt frameNum = 0;
//    TInt frameSize = 0;

    TPtr8 outAudioPtr(iOutAudioBuffer->Des());
//    const TInt frameSamples = 160;    
//    const TInt samplingRate = KVedAudioSamplingRate8k;

    TInt audioSampleDurationInTicks = 0;
    TPtrC8 writeDes;
    TUint8* frameBuffer = (TUint8*)outAudioPtr.Ptr();
    
    // Gets Size of buffer of current frames
	sampleSize = outAudioPtr.Length();
	
    // if frame is sampled at a different rate 
	audioSampleDurationInTicks = GetAudioTimeInTicksFromMs(iTotalDurationInSample/1000);       

    // set descriptor to sample
    writeDes.Set( outAudioPtr.Left(sampleSize) );	
    
    // compose audio to output 3gp file
    error = iComposer->WriteFrames((TDesC8&)writeDes, sampleSize, audioSampleDurationInTicks, 0 /*iAudioKeyFrame*/, 
        iAudioFramesInBuffer, CMP4Composer::EFrameTypeAudio);        
        
    iTotalDurationInSample = 0; // resetting the value in sample
    
    if (error != KErrNone)
        return error;

    outAudioPtr.SetLength(0);
	iAudioFrameNumber += iAudioFramesInBuffer;
    iAudioFramesInBuffer = 0;


    return KErrNone;

}

// -----------------------------------------------------------------------------
// CMovieProcessorImpl::SetFrameType
// Sets the frame type (inter/intra) for a frame
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CMovieProcessorImpl::SetFrameType(TInt aFrameIndex, TUint8 aType)
{
	VPASSERT(iFrameParameters);
	iFrameParameters[aFrameIndex].iType = aType;
}

// -----------------------------------------------------------------------------
// CMovieProcessorImpl::GetVideoTimeInMsFromTicks
// Converts a video timestamp from ticks to milliseconds
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
TInt64 CMovieProcessorImpl::GetVideoTimeInMsFromTicks(TInt64 aTimeStampInTicks, TBool aCommonTimeScale) const
{
    TUint timeScale = aCommonTimeScale ? iOutputVideoTimeScale : iParser->iStreamParameters.iVideoTimeScale; 
	VPASSERT(timeScale > 0);
    return TInt64( I64REAL(aTimeStampInTicks) / (TReal)timeScale * 1000 + 0.5 );
}

// -----------------------------------------------------------------------------
// CMovieProcessorImpl::GetVideoTimeInTicksFromMs
// Converts a video timestamp from ms to ticks
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
TInt64 CMovieProcessorImpl::GetVideoTimeInTicksFromMs(TInt64 aTimeStampInMs, TBool aCommonTimeScale) const
{
    TUint timeScale = aCommonTimeScale ? iOutputVideoTimeScale : iParser->iStreamParameters.iVideoTimeScale; 
        
	VPASSERT(timeScale > 0);
	
    return TInt64( I64REAL(aTimeStampInMs) * (TReal)timeScale / 1000 + 0.5 );
}

// -----------------------------------------------------------------------------
// CMovieProcessorImpl::GetAudioTimeInMsFromTicks
// Converts an audio timestamp from ticks to milliseconds
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
TUint CMovieProcessorImpl::GetAudioTimeInMsFromTicks(TUint aTimeStampInTicks) const
{

	TUint timeScale = 0;
	if(iParser)
	{
		timeScale = iParser->iStreamParameters.iAudioTimeScale; 	
	}
	if(timeScale == 0)
	{
		//means no audio in clip use output audio timescale itself	
		timeScale = iOutputAudioTimeScale;
	}
	VPASSERT(timeScale > 0);
	return TUint( (TReal)aTimeStampInTicks / (TReal)timeScale * 1000 + 0.5 ); 	
}


// -----------------------------------------------------------------------------
// CMovieProcessorImpl::GetAudioTimeInTicksFromMs
// Converts an audio timestamp from milliseconds to ticks
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
TUint CMovieProcessorImpl::GetAudioTimeInTicksFromMs(TUint aTimeStampInMs) const

{
	return TUint( (TReal)aTimeStampInMs * (TReal)iOutputAudioTimeScale / 1000.0 + 0.5 ); 
}

// -----------------------------------------------------------------------------
// CMovieProcessorImpl::WriteVideoFrameToFile
// Writes a video frame to output 3gp file
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
TInt CMovieProcessorImpl::WriteVideoFrameToFile(TDesC8& aBuf, TInt64 aTimeStampInTicks, 
                                                TInt /*aDurationInTicks*/, TBool aKeyFrame,
                                                TBool aCommonTimeScale, TBool aColorTransitionFlag,
                                                TBool aFromEncoder)
{    

    //TReal duration = TReal(aDurationInTicks);
	TBool lessThenZero = false;

	if (iDiskFull)
        return KErrDiskFull;

    // check available disk space    

    TInt64 freeSpace = 0;
	TInt error;
    // get free space on disk
    TRAP(error, freeSpace = iComposer->DriveFreeSpaceL());
    if (error != KErrNone)
        return error;

	// subtract metadata length from free space
    freeSpace -= TInt64(CurrentMetadataSize());

    if (freeSpace < TInt64(KDiskSafetyLimit))
    {
        PRINT((_L("CMovieProcessorImpl::WriteVideoFrameToFile() freeSpace below limit")))        
        iDiskFull = ETrue;
        return KErrDiskFull;
    }

    // check if we are writing the last frames of the previous
    // clip while doing crossfade / wipe, iFirstFrameOfClip should
    // be true when writing the first frame of current clip    
    if ( iCurClipTimeStampList.Count() )        
    {
        iFirstFrameOfClip = EFalse;
        if ( aColorTransitionFlag && !iFirstFrameFlagSet )
        {            
            // this frame is the first one of the second clip
            iFirstFrameOfClip = ETrue;
            iFirstFrameFlagSet = ETrue;
        }                
    }

    TVideoType vt = (TVideoType)iOutputVideoType;
	// This may be wrong as the files may be of different types so Mp4Specific size may be required to obtained for every file 
	// or every clip but only if it is the first frame	
	if (iVideoFrameNumber == 0 || iFirstFrameOfClip)
	{
		if (!iNumberOfVideoClips)
		{
		    // all clips were either from generator or were black frames
		    if ( iAllGeneratedClips )
	        {
    			iMP4SpecificSize = 0;
    			if (vt == EVideoMPEG4)
    			{
    				iModeChanged = ETrue;
    				iFirstClipUsesEncoder = ETrue;
    			}
	        }
	        else
            {
                // black frames; no parser or decoder exists; 
                // no need to modify the vos header by the decoder, 
                // but this forces the composer to search for the VOS header in the 1st frame data
				iModeChanged = ETrue;
            }
    	}

		else
		{
	        // VOS header size is parsed in composer for MPEG-4, in H.263 it is ignored
	        iMP4SpecificSize = 0;
		}
	}    
    
    // IMPORTANT: need to make decision here whether to change VosBit 
    // before writing to file, if first frame was encoded
       
    if (vt == EVideoMPEG4)
    {
        if(iVideoFrameNumber == 0)
        {
            if(iMpeg4ModeTranscoded)
            {
                TBool sBitChanged = EFalse;
                TRAP(error, sBitChanged = iVideoProcessor->CheckVosHeaderL((TPtrC8&)aBuf));
                if(sBitChanged) { }
                PRINT((_L("CMovieProcessorImpl::WritVideoFrameToFile() bit changed: %d"), sBitChanged))
                    if (error != KErrNone)
                        return error;
            }
        }
    }
    
    TInt currentMetaDataSize = CurrentMetadataSize();
    TInt64 oldVideoTime = GetVideoTimeInMsFromTicks((TInt64) iCurrentVideoTimeInTicks, ETrue);

    if (iFrameBuffered)
    {
        // write buffered frame to file
        
        TInt64 durationMs = GetVideoTimeInMsFromTicks(aTimeStampInTicks, aCommonTimeScale) - iBufferedTimeStamp;        

        if (iWriting1stColorTransitionFrame)
        {
            durationMs = i1stColorTransitionFrameTS - iBufferedTimeStamp;    
            iWriting1stColorTransitionFrame = EFalse;
        }
        
        else if (iFirstFrameOfClip)
        {            
            if ( iCurClipTimeStampList.Count() == 0 )
            {                
                VPASSERT(iVideoClipNumber > 0);
                CVedMovieImp* movie = reinterpret_cast<CVedMovieImp*>(iMovie);                                            
                // first frame of new clip            
                durationMs = movie->VideoClip(iVideoClipNumber-1)->CutOutTime().Int64()/1000 - iBufferedTimeStamp;
            }            
        }

        if (durationMs < 0)
        {
			lessThenZero = true;
            CVedMovieImp* movie = reinterpret_cast<CVedMovieImp*>(iMovie);            
            TReal frameRate = (movie->VideoFrameRate() > 0.0) ? movie->VideoFrameRate() : 15.0;
            durationMs =  TInt64( ( 1000.0 / frameRate ) + 0.5 );            
        }

        if ( iNumberOfVideoClips )
        {            
            TInt64 clipDuration = iVideoClip->CutOutTime().Int64()/1000 -
                                  iVideoClip->CutInTime().Int64()/1000;

            if ( iVideoClipNumber == iNumberOfVideoClips - 1 )
            {
                // Add duration of possible black ending frames
                CVedMovieImp* movie = reinterpret_cast<CVedMovieImp*>(iMovie);
                clipDuration += ( movie->Duration().Int64()/1000 - 
                                  movie->VideoClip(iNumberOfVideoClips-1)->EndTime().Int64()/1000 );
            }
            
            if ( iVideoClipWritten + durationMs > clipDuration )
            {
                durationMs = clipDuration - iVideoClipWritten;
                
                if ( durationMs <= 0 )
                {
                    TPtr8 outVideoPtr(iOutVideoBuffer->Des());
                    outVideoPtr.SetLength(0);
                    iFrameBuffered = EFalse;
                    return KErrCompletion;
                }
            }
        }
        
        TReal duration = I64REAL(durationMs) * TReal(iOutputVideoTimeScale) / 1000.0;
        
        error = WriteVideoFrameFromBuffer(duration, aColorTransitionFlag);
        
        if (error != KErrNone)
            return error;
    }
    
    TInt64 currentVideoTime = GetVideoTimeInMsFromTicks((TInt64) iCurrentVideoTimeInTicks, ETrue);
    
    iCurrentVideoSize += aBuf.Length();
    
    TInt addAudio = 0;    
    TRAP( error, addAudio = (iMovie->Song())->GetFrameSizeEstimateL(oldVideoTime * 1000, currentVideoTime * 1000) );
    
    if (error != KErrNone)
        return error;
    
    iCurrentAudioSize += addAudio;
    
    PRINT((_L("CMovieProcessorImpl::WriteVideoFrameToFile() video size: %d, audio size %d, meta data %d"), iCurrentVideoSize, iCurrentAudioSize, currentMetaDataSize))
    
    if (iMovieSizeLimit > 0 && iCurrentVideoSize + iCurrentAudioSize + currentMetaDataSize > iMovieSizeLimit)
    {
        // Cut video here
        iEndCutTime = TTimeIntervalMicroSeconds(currentVideoTime);
        
        iMovieSizeLimitExceeded = ETrue;
    
        // To notify that movie has reached maximum size
        return KErrCompletion;
    }

    // Buffer frame

    VPASSERT(iOutVideoBuffer); 
    TPtr8 outVideoPtr(iOutVideoBuffer->Des());
    
    if((outVideoPtr.Length() + aBuf.Length()) > outVideoPtr.MaxLength()) 
    {
        // extend buffer size
                
        TUint newSize = outVideoPtr.Length() + aBuf.Length();
        // round up to the next full kilobyte       
        newSize = (newSize + 1023) & (~1023);
        TRAP(error, (iOutVideoBuffer = iOutVideoBuffer->ReAllocL(newSize)) );
        
        if (error != KErrNone)
            return error;
        
        PRINT((_L("CMovieProcessorImpl::WriteVideoFrameToFile() - extended buffer to %d bytes"), newSize));
        
        outVideoPtr.Set(iOutVideoBuffer->Des());
    }        
    outVideoPtr.Append(aBuf);
    
#ifdef VIDEOEDITORENGINE_AVC_EDITING
    if (vt == EVideoAVCProfileBaseline)
    {        
//        TBool containsDCR = EFalse;
//        if (!aFromEncoder && iFirstFrameOfClip)
//            containsDCR = ETrue;
        
	    error = iAvcEdit->ParseFrame(iOutVideoBuffer, EFalse, aFromEncoder);
	    if (error != KErrNone)
            return error;
    }
#endif

    if(!lessThenZero)
    	{
    	iBufferedTimeStamp = GetVideoTimeInMsFromTicks(aTimeStampInTicks, aCommonTimeScale);	
    	}
    iBufferedKeyFrame = aKeyFrame;
    iBufferedFromEncoder = aFromEncoder;
    iFrameBuffered = ETrue;
    if ( iFirstFrameOfClip )
        iFirstFrameBuffered = ETrue;

    iFirstFrameOfClip = EFalse;

    iVideoFrameNumber++;

    if (aKeyFrame)
        iVideoIntraFrameNumber++;	

	return KErrNone;

}

// -----------------------------------------------------------------------------
// CMovieProcessorImpl::FinalizeVideoWrite
// Writes the last video frame from buffer to file
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
TInt CMovieProcessorImpl::FinalizeVideoWrite()
{

    if ( !iFrameBuffered )
        return KErrNone;

    CVedMovieImp* movie = reinterpret_cast<CVedMovieImp*>(iMovie);
    
    iApplySlowMotion = EFalse;  // this duration must not be scaled
    TInt64 durationMs = movie->Duration().Int64()/1000 - GetVideoTimeInMsFromTicks(iCurrentVideoTimeInTicks, ETrue);
    TReal duration = I64REAL(durationMs) * TReal(iOutputVideoTimeScale) / 1000.0;

    TInt error = WriteVideoFrameFromBuffer(duration, EFalse);

    return error;
}

// -----------------------------------------------------------------------------
// CMovieProcessorImpl::FinalizeVideoWrite
// Writes a frame from buffer to file
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
TInt CMovieProcessorImpl::WriteVideoFrameFromBuffer(TReal aDuration, TBool aColorTransitionFlag)
{

    VPASSERT(iFrameBuffered);

    TReal duration = aDuration;

    // slow motion
    VPASSERT(iSpeed > 0);			
    const TInt maxSpeed = KMaxVideoSpeed; 
    TReal iScaleFactor = (TReal)maxSpeed/(TReal)iSpeed;
    
    if( iScaleFactor != 1.0 && iApplySlowMotion )
    {
        duration = duration * iScaleFactor;
    }
    
    iCurrentVideoTimeInTicks += duration;

    // if the first frame of a clip is being buffered,
    // we are now writing the last frame of previous clip
    // so don't increment iVideoClipWritten yet
    
    // Also take into account crossfade / wipe transition
    // so that iVideoClipWritten is not incremented
    // for transition frames of the previous clip
    if ( !iFirstFrameOfClip && ( (iCurClipTimeStampList.Count() == 0) || 
                                  aColorTransitionFlag ) )
    {        
        iVideoClipWritten += GetVideoTimeInMsFromTicks(aDuration, ETrue);        
    }

    IncProgressBar();
    
    TVedVideoBitstreamMode currClipMode;
    TVideoType vt = (TVideoType)iOutputVideoType;
    
    if (iNumberOfVideoClips > 0)
        currClipMode = iVideoClip->Info()->TranscodeFactor().iStreamType;	
    else
    {
        if ( (vt == EVideoH263Profile0Level10) || (vt == EVideoH263Profile0Level45) )
            currClipMode = EVedVideoBitstreamModeH263;
        else
            currClipMode = EVedVideoBitstreamModeMPEG4Regular;
    }
    TPtr8 outVideoPtr(iOutVideoBuffer->Des());
    
    TInt returnedVal = 0;
    returnedVal = iComposer->WriteFrames(outVideoPtr, outVideoPtr.Size(), TInt(duration + 0.5), 
        iBufferedKeyFrame, 1 /*numberOfFrames*/, 
        CMP4Composer::EFrameTypeVideo, iMP4SpecificSize, 
        iModeChanged, iFirstFrameBuffered, currClipMode, iBufferedFromEncoder);	

    iFirstFrameBuffered = EFalse;
    
    if (returnedVal == KErrWrite)
    {
        // frame was not written
        iVideoClipWritten -= GetVideoTimeInMsFromTicks(aDuration, ETrue);
        iCurrentVideoTimeInTicks -= duration;
        returnedVal = KErrNone;
    }    
    
    iFrameBuffered = EFalse;
    outVideoPtr.SetLength(0);
    
    return returnedVal;    

}


// -----------------------------------------------------------------------------
// CMovieProcessorImpl::SaveVideoFrameToFile
// Save a video YUV frame to the tmp file
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
TInt CMovieProcessorImpl::SaveVideoFrameToFile(TDesC8& aBuf, TInt aDuration, TInt64 aTimeStamp )
{
    TBool isOpen = EFalse;
    TInt errCode = KErrNone;
    if ( ( errCode = iFs.IsFileOpen( iNextClipFileName, isOpen ) ) == KErrNone )
    {
        if ( isOpen )
        {
            if ( ( errCode = iNextClipDurationList.Append( aDuration ) ) == KErrNone )
            {
                if ( ( errCode = iNextClipTimeStampList.Append( aTimeStamp ) ) == KErrNone )
                {
                    if ( iDiskFull )
                    {
                        errCode = KErrDiskFull;                    
                    }
                    else
                    {
                        TInt64 freeSpace = 0;
                        // get free space on disk
                        TRAP( errCode, freeSpace = iComposer->DriveFreeSpaceL() );
                        if ( errCode == KErrNone )
                        {
                            // subtract yuv length from free space
                            freeSpace -= TInt64( aBuf.Length() );
							
                            if ( freeSpace < TInt64( KDiskSafetyLimit ) )
                            {
                                iDiskFull = ETrue;
                                errCode = KErrDiskFull;
                            }
                            else
                            {
                                errCode = iNextClip.Write( aBuf );
                                iPreviousTimeScale = iParser->iStreamParameters.iVideoTimeScale;
                            }
                        }
                    }
                    if ( errCode != KErrNone )
                    {
                        // rollback the insertion
                        iNextClipDurationList.Remove( iNextClipDurationList.Count() - 1 );
                        iNextClipTimeStampList.Remove( iNextClipTimeStampList.Count() - 1 );
                    }
                }
                else
                {
                    // rollback the insertion
                    iNextClipDurationList.Remove( iNextClipDurationList.Count() - 1 );
                }
            }
        }
        else
        {
            errCode = KErrNotFound;
        }
    }
    return errCode;
}

// -----------------------------------------------------------------------------
// CMovieProcessorImpl::GetVideoFrameFromFile
// Retrieve a video YUV frame from the tmp file
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
TInt CMovieProcessorImpl::GetVideoFrameFromFile(TDes8& aBuf, TInt aLength, TInt& aDuration, TInt64& aTimeStamp)
{
    
    TBool isOpen = EFalse;
    TInt errCode = KErrNone;
    if ( ( errCode = iFs.IsFileOpen( iCurClipFileName, isOpen ) ) == KErrNone )
    {
        if ( isOpen )
        {
            if ( ( errCode = iCurClip.Read( aBuf, aLength ) ) == KErrNone )
            {
                if ( !iTimeStampListScaled )
                {                
                    // change timestamps to start from zero
                    TInt firstTs = iCurClipTimeStampList[0];
                    for ( TInt index = 0; index < iCurClipTimeStampList.Count(); ++index )       
                    {
                        iCurClipTimeStampList[index] = iCurClipTimeStampList[index] - firstTs;
                    }
                
                    iOffsetTimeStamp = iCurClipTimeStampList[iCurClipTimeStampList.Count() - 1] + 
						iCurClipDurationList[iCurClipDurationList.Count() - 1];
                    TReal scaleFactor = (TReal)iParser->iStreamParameters.iVideoTimeScale/(TReal)iPreviousTimeScale;
                    iOffsetTimeStamp = TInt( I64REAL(iOffsetTimeStamp) * scaleFactor + 0.5 );
					
                    for ( TInt index = 0; index < iCurClipDurationList.Count(); ++index )
                    {
                        // scale up or scale down
                        iCurClipDurationList[index] = TInt( TReal(iCurClipDurationList[index]) * scaleFactor + 0.5 );
                        iCurClipTimeStampList[index] = TInt64( I64REAL(iCurClipTimeStampList[index]) * scaleFactor + 0.5 );
                    }
                    iTimeStampListScaled = ETrue;
                }
                
                if ( iCurClipIndex < iCurClipDurationList.Count() )
                {
                    aDuration = iCurClipDurationList[iCurClipIndex];
                    aTimeStamp = iCurClipTimeStampList[iCurClipIndex++];
                }
                else
                {
                    aDuration = -1;
                    aTimeStamp = -1;
                }
                
            }
        }
        else
        {
            errCode = KErrNotFound;
        }
    }
    return errCode;
}


// -----------------------------------------------------------------------------
// CMovieProcessorImpl::GetNextFrameDuration
// Get the next frame duration and timestamp
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CMovieProcessorImpl::GetNextFrameDuration(TInt& aDuration, TInt64& aTimeStamp, 
                                               TInt aIndex, TInt& aTimeStampOffset)
{

    TInt count = iCurClipTimeStampList.Count();
    
    if (!iTimeStampListScaled)
    {
        // save the timestamp of first color transition frame, so that the duration of
        // last frame from first clip is calculated correctly in WriteVideoFrameToFile()        
        i1stColorTransitionFrameTS = TInt64( I64REAL(iCurClipTimeStampList[0]) / (TReal)iPreviousTimeScale * 1000 + 0.5 );
        iWriting1stColorTransitionFrame = ETrue;
    
        // change timestamps to start from zero
        TInt firstTs = iCurClipTimeStampList[0];
        for ( TInt index = 0; index < iCurClipTimeStampList.Count(); ++index )       
        {
            iCurClipTimeStampList[index] = iCurClipTimeStampList[index] - firstTs;
        }
    
        iOffsetTimeStamp = iCurClipTimeStampList[iCurClipTimeStampList.Count() - 1] + 
				 		   iCurClipDurationList[iCurClipDurationList.Count() - 1];
        TReal scaleFactor = (TReal)iParser->iStreamParameters.iVideoTimeScale/(TReal)iPreviousTimeScale;
        iOffsetTimeStamp = TInt( I64REAL(iOffsetTimeStamp) * scaleFactor + 0.5 );
					
        for ( TInt index = 0; index < iCurClipDurationList.Count(); ++index )
        {
            // scale up or scale down
            iCurClipDurationList[index] = TInt( TReal(iCurClipDurationList[index]) * scaleFactor + 0.5 );
            iCurClipTimeStampList[index] = TInt64( I64REAL(iCurClipTimeStampList[index]) * scaleFactor + 0.5 );
        }     
        iTimeStampListScaled = ETrue;
    }
    
    aTimeStampOffset = iOffsetTimeStamp;

    if (aIndex >= 0)
    {
        // just get the values for given index, do not change iCurClipIndex
        VPASSERT(aIndex < iCurClipDurationList.Count());
        aDuration = iCurClipDurationList[aIndex];
        aTimeStamp = iCurClipTimeStampList[aIndex];
        return;
    }

    if ( iCurClipIndex < iCurClipDurationList.Count() )
    {
        aDuration = iCurClipDurationList[iCurClipIndex];
        aTimeStamp = iCurClipTimeStampList[iCurClipIndex++];
    }
    else
    {
        aDuration = 0;
        aTimeStamp = 0;
    }
}


// -----------------------------------------------------------------------------
// CMovieProcessorImpl::AppendNextFrameDuration
// Append the next frame duration and timestamp
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CMovieProcessorImpl::AppendNextFrameDuration(TInt aDuration, TInt64 aTimeStamp)
{
    if ( iCurClipDurationList.Append( aDuration ) == KErrNone )
    {
        if ( iCurClipTimeStampList.Append( aTimeStamp + iOffsetTimeStamp ) != KErrNone )
        {
            // rollback the insertion
            iCurClipDurationList.Remove( iCurClipDurationList.Count() - 1 );
        }
    }    
}

// -----------------------------------------------------------------------------
// CMovieProcessorImpl::GetNextClipStartTransitionNumber
// Get the number of transition at the start of next clip
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
TInt CMovieProcessorImpl::NextClipStartTransitionNumber()
{
    TInt transitionNumber = 5;
    if ( iMiddleTransitionEffect == EVedMiddleTransitionEffectCrossfade ||
         iMiddleTransitionEffect == EVedMiddleTransitionEffectWipeLeftToRight ||
         iMiddleTransitionEffect == EVedMiddleTransitionEffectWipeRightToLeft ||
         iMiddleTransitionEffect == EVedMiddleTransitionEffectWipeTopToBottom ||
         iMiddleTransitionEffect == EVedMiddleTransitionEffectWipeBottomToTop )
    {
        TInt nextClipNumber = iVideoClipNumber + 1;
        CVedMovieImp* movie = (iMovie);
        CVedVideoClip* videoClip = movie->VideoClip(nextClipNumber);  
        CVedVideoClip* origVideoClip = iVideoClip;

        TVedMiddleTransitionEffect middleTransitionEffect = EVedMiddleTransitionEffectNone;
        if ( iMovie->MiddleTransitionEffectCount() > nextClipNumber )
        {
            middleTransitionEffect = iMovie->MiddleTransitionEffect( nextClipNumber );
        }

        TTimeIntervalMicroSeconds startCutTime = videoClip->CutInTime();
        TTimeIntervalMicroSeconds endCutTime   = videoClip->CutOutTime();

        TInt numberOfFrame = videoClip->Info()->VideoFrameCount();

        // temporary set iVideoClip to next video clip
        iVideoClip = videoClip;

        TInt startFrameIndex = GetVideoFrameIndex( startCutTime );
        // the following is because binary search gives us frame with timestamp < startCutTime
        // this frame would be out of range for  movie
        if ( startFrameIndex > 0 && startFrameIndex < ( numberOfFrame - 1 ) )
            startFrameIndex++;

        TInt endFrameIndex = GetVideoFrameIndex( endCutTime );

        // determine the total number of included frames in the clip
        TInt numberOfIncludedFrames = endFrameIndex - startFrameIndex + 1;
                        
        // make sure there are enough frames to apply transition
        // for transition at both ends
        if ( middleTransitionEffect != EVedMiddleTransitionEffectNone )
        {
            if ( middleTransitionEffect == EVedMiddleTransitionEffectCrossfade ||
                 middleTransitionEffect == EVedMiddleTransitionEffectWipeLeftToRight ||
                 middleTransitionEffect == EVedMiddleTransitionEffectWipeRightToLeft ||
                 middleTransitionEffect == EVedMiddleTransitionEffectWipeTopToBottom ||
                 middleTransitionEffect == EVedMiddleTransitionEffectWipeBottomToTop )
            {
                if ( numberOfIncludedFrames < ( transitionNumber << 1 ) )
                {
                    transitionNumber = numberOfIncludedFrames >> 1;
                }
            }
            else
            {
                if ( numberOfIncludedFrames < ( transitionNumber + 10 ) )
                {
                    if ( ( numberOfIncludedFrames >> 1 ) < transitionNumber )
                    {
                        transitionNumber = numberOfIncludedFrames >> 1;
                    }
                }
            }
        }
        else 
        {
            if ( numberOfIncludedFrames < transitionNumber )
            {
                transitionNumber = numberOfIncludedFrames;
            }
        }
        // reset iVideoClip back to the original video clip
        iVideoClip = origVideoClip;
    }
    return transitionNumber;   
}

// -----------------------------------------------------------------------------
// CMovieProcessorImpl::TransitionDuration
// Get the transition duration of current clip
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
TInt CMovieProcessorImpl::TransitionDuration()
{
    TInt duration = 0;
    if ( iNextClipDurationList.Count() )
    {
        for ( TInt index = 0; index < iNextClipDurationList.Count() /*- 1*/; index++ )
        {
            duration += iNextClipDurationList[index];
        }
    }
    return duration;
}

// -----------------------------------------------------------------------------
// CMovieProcessorImpl::CloseTransitionInfoL
// Release all internal data hold for transition effect
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CMovieProcessorImpl::CloseTransitionInfoL()
{
    // remove old tmp files
    if ( iCurClipFileName.Length() )
    {
        iCurClip.Close();
        // it is ok if the tmp file does not exist
        iFs.Delete( iCurClipFileName );
        iCurClipFileName.Zero();
    }

    if ( iNextClipFileName.Length() )
    {
        iNextClip.Close();
        // it is ok if the tmp file does not exist
        iFs.Delete( iNextClipFileName );
        iNextClipFileName.Zero();
    }

    // use file manager to delete all previously
    // *_nokia_vpi.tmp files
    CFileMan *fileMan = CFileMan::NewL( iFs );
    CleanupStack::PushL( fileMan );
    TParse filepath;
    filepath.Set( iOutputMovieFileName, NULL, NULL );
    TFileName filesToDelete = filepath.DriveAndPath();
    filesToDelete.Append( _L( "*_nokia_vpi.tmp" ) );    
    fileMan->Delete( filesToDelete );
    CleanupStack::PopAndDestroy( fileMan );

    iCurClipDurationList.Reset();
    iNextClipDurationList.Reset();
    iCurClipTimeStampList.Reset();
    iNextClipTimeStampList.Reset();
    iCurClipIndex = 0;
    iPreviousTimeScale = 0;
    iOffsetTimeStamp = 0;    
		
    // Delete Im_nokia_vpi.tmp
	TFileName tmpath = TPtrC(KTempFilePath);         
    tmpath.Append( _L("x.3gp") );
	
    // use file manager to delete all previously
	CFileMan *tempfileMan = CFileMan::NewL( iFs );
	CleanupStack::PushL( tempfileMan );
    TParse tempfilepath;
    tempfilepath.Set(tmpath, NULL, NULL );
    TFileName tempfilesToDelete = tempfilepath.DriveAndPath();
    tempfilesToDelete.Append( _L( "Im_nokia_vpi.tmp" ) );
    tempfileMan->Delete( tempfilesToDelete,0);
    CleanupStack::PopAndDestroy( tempfileMan );
}

// -----------------------------------------------------------------------------
// CMovieProcessorImpl::GetVideoFrameIndex
// Gets frame index based on its timestamp
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
TInt CMovieProcessorImpl::GetVideoFrameIndex(TTimeIntervalMicroSeconds aTime) const
{
	__ASSERT_ALWAYS((aTime >= TTimeIntervalMicroSeconds(0)) /*&& (aTime <= iMovie->Duration())*/, 
			        TVedPanic::Panic(TVedPanic::EVideoClipInfoIllegalVideoFrameTime));

	/* Use binary search to find the right frame. */

    TInt videoFrameCount;
    if (!iThumbnailInProgress)
	    videoFrameCount = iVideoClip->Info()->VideoFrameCount();
	else
	    videoFrameCount = iParser->GetNumberOfVideoFrames();
	
	TInt start = 0;
	TInt end = videoFrameCount - 1;
	TInt index = -1;
	while ( start <= end )
		{
		index = start + ((end - start) / 2);

		TTimeIntervalMicroSeconds startTime(0);
		TTimeIntervalMicroSeconds endTime(0);
				
	    TInt tsInTicks;
	    startTime = TTimeIntervalMicroSeconds( TInt64(iParser->GetVideoFrameStartTime(index, &tsInTicks)) * TInt64(1000) );
	    
	    if (index < (videoFrameCount - 1))
	        endTime = TTimeIntervalMicroSeconds( TInt64(iParser->GetVideoFrameStartTime(index + 1, &tsInTicks)) * TInt64(1000) );
	    else
	    {
	       TInt durationMs;   
	       iParser->GetVideoDuration(durationMs);		       
	       endTime = TTimeIntervalMicroSeconds( TInt64(durationMs) * TInt64(1000) );
	    }	
		
		if (index < (videoFrameCount - 1))
			{
			endTime = TTimeIntervalMicroSeconds(endTime.Int64() - 1);
			}

		if (aTime < startTime)
			{
			end = index - 1;
			}
		else if (aTime > endTime)
			{
			start = index + 1;
			}
		else
			{
			break;
			}
		}

	return index;
}


// -----------------------------------------------------------------------------
// CMovieProcessorImpl::EnhanceThumbnailL
// Enhances the visual quality of the frame
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CMovieProcessorImpl::EnhanceThumbnailL(const CFbsBitmap* aInBitmap, 
																						CFbsBitmap* aTargetBitmap) 
{

  // create enhancement object
	if(!iEnhancer)
    iEnhancer = (CDisplayChain*) CDisplayChain::NewL();

	// enhance image
	iEnhancer->ProcessL(aInBitmap, aTargetBitmap); 

	// clear enhancement object
	delete iEnhancer;
	iEnhancer=0;

}


// -----------------------------------------------------------------------------
// CMovieProcessorImpl::NotifyVideoClipGeneratorFrameCompleted
// The cal back functin called when a frame is to be obtained from the frame generator
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CMovieProcessorImpl::NotifyVideoClipGeneratorFrameCompleted(CVedVideoClipGenerator& /*aGenerator*/, 
                                                                 TInt aError, 
                                                                 CFbsBitmap* aFrame)
{

    if (iProcessingCancelled)
    {
        if (aFrame)
        {
            delete aFrame; aFrame = 0;
        }
        return;
    }

	if(aError == KErrNone)
	{	
		CFbsBitmap* aBitmapLocal = aFrame;

        if (aFrame == 0)
        {
            iMonitor->Error(KErrArgument);
            return;
        }

        VPASSERT(aFrame);
        TInt error;

		// Convert the frame obtained from bitmap to YUV before giving it to encoder only if error is not there
		TRAP(error, iImageYuvConverter = CVSFbsBitmapYUV420Converter::NewL(*aBitmapLocal));
        if (error != KErrNone)
        {
            delete aFrame; aFrame = 0;	// removed the frame obtained from engine                        
            iMonitor->Error(error);
            return;
        }

		VPASSERT(iImageYuvConverter);		

		TRAP(error, iImageYuvConverter->ProcessL());
        if (error != KErrNone)
        {            
            delete aFrame; aFrame = 0;	// removed the frame obtained from engine                                    
            iMonitor->Error(error);
            return;
        }

		TRAP(error, iYuvImageBuf = (TUint8*) (TUint8*) HBufC::NewL((iImageYuvConverter->YUVData()).Length()));
		if(error != KErrNone)
		{
            delete aFrame; aFrame = 0;	// removed the frame obtained from engine                       
            iMonitor->Error(error);
            return;			
		}
		iImageSize = (iImageYuvConverter->YUVData()).Length();
		Mem::Copy(iYuvImageBuf,(iImageYuvConverter->YUVData()).Ptr(),(iImageYuvConverter->YUVData()).Length());	// may be iYUVBuf only	
		iReadImageDes.Set(iYuvImageBuf,iImageSize, iImageSize);
				
		delete iImageYuvConverter;
		iImageYuvConverter = 0;
		
		aBitmapLocal = 0;
		if(aFrame)
		{
			delete aFrame;		// removed the frame obtained from engine
			aFrame = 0;
		}
		aBitmapLocal = 0;
		
		// finished converting to YUV 
		TRAP(error, ProcessImageSetsL(EVideoEncodeFrame)); // EVideoEncodeFrame indicates to encoder to encode and return later
        if(error != KErrNone)
		{            
            iMonitor->Error(error);
            return;			
		}
	}
	else
	{
		if(aError == KErrCancel)
		{
			if(aFrame)
			{
				delete aFrame;
				aFrame =0;
			}						
			
			if(iImageComposer)
			{
				iImageComposer->Close();
				delete iImageComposer;						
				iImageComposer=0;
			}
			
			if (iImageAvcEdit)
        	{
        	    delete iImageAvcEdit;
        	    iImageAvcEdit = 0;
        	}
		}
		else
		{
			iMonitor->Error(aError);
		}
	}
}

// -----------------------------------------------------------------------------
// CMovieProcessorImpl::GetOutputVideoType			
// This function returns the video type of output movie
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
TVedVideoType CMovieProcessorImpl::GetOutputVideoType()
{

	if (iOutputVideoType == EVideoH263Profile0Level10)
		return EVedVideoTypeH263Profile0Level10;

	if (iOutputVideoType == EVideoH263Profile0Level45)
		return EVedVideoTypeH263Profile0Level45;
	
#ifdef VIDEOEDITORENGINE_AVC_EDITING
    if (iOutputVideoType == EVideoAVCProfileBaseline)
		return EVedVideoTypeAVCBaselineProfile;
#endif

	if (iOutputVideoType == EVideoMPEG4)
		return EVedVideoTypeMPEG4SimpleProfile;

	return EVedVideoTypeNoVideo;

}

// -----------------------------------------------------------------------------
// CMovieProcessorImpl::GetCurrentClipVideoType
// This function returns the video type of the current video clip 
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
TVedVideoType CMovieProcessorImpl::GetCurrentClipVideoType()
{

    if (!iThumbnailInProgress)
    {
        
        CVedMovieImp* movie = reinterpret_cast<CVedMovieImp*>(iMovie);

        CVedVideoClip* currentClip = movie->VideoClip(iVideoClipNumber);
        CVedVideoClipInfo* currentInfo = currentClip->Info();

        if (currentInfo->Class() == EVedVideoClipClassGenerated)
        {
            switch (iOutputVideoType)
            {
            	case EVideoH263Profile0Level10:
            	    return EVedVideoTypeH263Profile0Level10;
//            	    break;
            	    
            	case EVideoH263Profile0Level45:
            	    return EVedVideoTypeH263Profile0Level45;
//            	    break;

#ifdef VIDEOEDITORENGINE_AVC_EDITING            	    
                case EVideoAVCProfileBaseline:
    		        return EVedVideoTypeAVCBaselineProfile;
//    		        break;
#endif
            	    
            	case EVideoMPEG4:
            	    return EVedVideoTypeMPEG4SimpleProfile;
//            	    break;
            	    
            	default:
            	    return EVedVideoTypeUnrecognized;
            }
        }        
        return currentInfo->VideoType();  
    }
    
    switch (iParser->iStreamParameters.iVideoFormat)
    {
    
        case CParser::EVideoFormatH263Profile0Level10:
            return EVedVideoTypeH263Profile0Level10;
//            break;
            
        case CParser::EVideoFormatH263Profile0Level45:
            return EVedVideoTypeH263Profile0Level45;
//            break;
            
        case CParser::EVideoFormatMPEG4:
            return EVedVideoTypeMPEG4SimpleProfile;
//            break;
                
        case CParser::EVideoFormatAVCProfileBaseline:
            return EVedVideoTypeAVCBaselineProfile;
//            break;
            
        default:
            return EVedVideoTypeUnrecognized;            
    }
}



// -----------------------------------------------------------------------------
// CMovieProcessorImpl::GetVideoClipTranscodeFactor	Added for transcoding reqs
// This function returns the mode of the clip indicated by the location aNum
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
TVedTranscodeFactor CMovieProcessorImpl::GetVideoClipTranscodeFactor(TInt aNum)
{
	CVedVideoClipInfo* curInfo = NULL;

	if(iMovie)
	{
		CVedMovieImp* tmovie = (iMovie);
		CVedVideoClip* currentClip = tmovie->VideoClip(aNum);
		CVedVideoClipInfo* currentInfo = currentClip->Info();
		curInfo = currentInfo;
	}
	else
	{
		// this means there is no movie, which can happen in case of thumb generation as processor does not have it
		User::Panic(_L("CMovieProcessorImpl MoviePtr is Missing in VideoProcessor"), -1);
	}
	
	return curInfo->TranscodeFactor();
}


// -----------------------------------------------------------------------------
// CMovieProcessorImpl::GetVideoClipResolution
// Panics the program on error
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
TSize CMovieProcessorImpl::GetVideoClipResolution()
{
	return iVideoClip->Info()->Resolution();
}

// -----------------------------------------------------------------------------
// CMovieProcessorImpl::GetMovieResolution
// Gets target movie resolution
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
TSize CMovieProcessorImpl::GetMovieResolution()
{
	if(iMovie)
	{
		return iMovie->Resolution();
	}
	else
	{
		TSize stdres(KVedMaxVideoWidth, KVedMaxVideoHeight);
		return stdres;
	}
}

// -----------------------------------------------------------------------------
// CMovieProcessorImpl::GetVideoClipFrameRate
// Gets video frame rate of current clip
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
TReal CMovieProcessorImpl::GetVideoClipFrameRate()
    {

    TReal rate;
    iParser->GetVideoFrameRate(rate);

    return rate;

    }

// -----------------------------------------------------------------------------
// CMovieProcessorImpl::GetMovieFrameRate
// Gets target movie frame rate
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
TReal CMovieProcessorImpl::GetMovieFrameRate()
    {
    return iMovie->VideoFrameRate();
    }

// -----------------------------------------------------------------------------
// CMovieProcessorImpl::GetMovieVideoBitrate
// Gets target movie video bit rate
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
TInt CMovieProcessorImpl::GetMovieVideoBitrate()
    {
    return iMovie->VideoBitrate();
    }
  
// -----------------------------------------------------------------------------
// CMovieProcessorImpl::GetMovieVideoBitrate
// Gets standard movie video bit rate (mandated by standard for used video codec)
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//  
TInt CMovieProcessorImpl::GetMovieStandardVideoBitrate()    
    {   
     return iMovie->VideoStandardBitrate();
    }

// -----------------------------------------------------------------------------
// CMovieProcessorImpl::GetMovieAudioBitrate
// Gets target movie audio bit rate
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
TInt CMovieProcessorImpl::GetMovieAudioBitrate()
    {
    return iMovie->AudioBitrate();
    }

// -----------------------------------------------------------------------------
// CMovieProcessorImpl::GetSyncIntervalInPicture
// Gets sync interval in picture
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
TInt CMovieProcessorImpl::GetSyncIntervalInPicture()
    {
    if ( iMovie )
        {
        return iMovie->SyncIntervalInPicture();
        }
    else
        {
        return 0;
        }
    }
    
// -----------------------------------------------------------------------------
// CMovieProcessorImpl::GetRandomAccessRate
// Get random access rate setting
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
TReal CMovieProcessorImpl::GetRandomAccessRate()
    {
    if ( iMovie )
        {
        return iMovie->RandomAccessRate();
        }
    else
        {
        return 0.2;
        }
    }

// -----------------------------------------------------------------------------
// CMovieProcessorImpl::GetOutputAudioType()			Added for AAC support
// This function returns the audio type of the final movie which will be created
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
TVedAudioType CMovieProcessorImpl::GetOutputAudioType()	//added for AAC support
{
	return DecideAudioType(iOutputAudioType);
}

// -----------------------------------------------------------------------------
// CMovieProcessorImpl::DecideAudioType(TAudioType aAudioType)			added for AAC support
// This function returns the audio type depending on the parameter sent
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
TVedAudioType CMovieProcessorImpl::DecideAudioType(TAudioType aAudioType)	//added for AAC support
{	
	if( aAudioType == (TAudioType)EAudioAMR)
	{
		return EVedAudioTypeAMR;
	}
	else if(aAudioType == (TAudioType)EAudioAAC)
	{
		// changed EVedAudioTypeAAC_LC --> EVedAudioTypeAAC_LC --Sami
		return EVedAudioTypeAAC_LC;
	}
	else if(aAudioType == (TAudioType)EAudioNone)
	{
		return EVedAudioTypeNoAudio;
	}
	else
	{
		return EVedAudioTypeUnrecognized;
	}
}

// -----------------------------------------------------------------------------
// CMovieProcessorImpl::SetMovieSizeLimit
// Sets the maximum size for the movie
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CMovieProcessorImpl::SetMovieSizeLimit(TInt aLimit)
{
    // reserve 2000 bytes for safety margin, since size is checked after a video frame is written to file.
    iMovieSizeLimit = (aLimit - 2000 > 0) ? aLimit - 2000 : 0;
}


//==================================
// color tone RGB TO YUV Function
//==================================

void CMovieProcessorImpl::ConvertColorToneRGBToYUV(TVedColorEffect aColorEffect,TRgb aColorToneRgb)
{
	TInt uVal, vVal; // to get the U,V Values of the ColorTone
	
	CVedVideoClipInfo* currentInfo = iVideoClip->Info();
	TVedVideoBitstreamMode streamMode = currentInfo->TranscodeFactor().iStreamType;

	// evaluate the u,v values for color toning
	if (aColorEffect == EVedColorEffectToning)						
	{
		uVal = TInt(-(0.16875*aColorToneRgb.Red()) - (0.33126*aColorToneRgb.Green()) + (0.5*aColorToneRgb.Blue()) + 0.5);
		vVal = TInt((0.5*aColorToneRgb.Red()) - (0.41869*aColorToneRgb.Green()) - (0.08131*aColorToneRgb.Blue()) + 0.5);
	}
	
	// adjust the u,v values for h.263 and mpeg-4
	if(iOutputVideoType == EVideoH263Profile0Level10 || iOutputVideoType == EVideoH263Profile0Level45 || (!(iOutputVideoType == EVideoMPEG4) && streamMode == EVedVideoBitstreamModeMPEG4ShortHeader))
	{
		if(aColorEffect == EVedColorEffectBlackAndWhite)
		{
			uVal = 255;  // codeword for value=128
			vVal = 255;  // codeword for value=128
		}
		else if (aColorEffect == EVedColorEffectToning)
		{
			uVal += 128;
			vVal += 128;
			
			AdjustH263UV(uVal);
			AdjustH263UV(vVal);
		}
	}
	else if (iOutputVideoType == EVideoMPEG4)
	{
		if(aColorEffect == EVedColorEffectBlackAndWhite)
		{
			uVal = 0;  // codeword for value=128
			vVal = 0;  // codeword for value=128
		}
		else if (aColorEffect == EVedColorEffectToning)
		{
			uVal /= 2;	// do not use bit shift; may have negative values
			vVal /= 2;	// do not use bit shift; may have negative values
		}
	}
	
	iColorToneU = uVal;
	iColorToneV = vVal;
//	iMovie->VideoClipSetColorTone(0, iColorToneYUV);

}

//=======================================
// CMovieProcessorImpl::AdjustH263UV()
// Adjusts the UV values for Color Toning
//=======================================
void CMovieProcessorImpl::AdjustH263UV(TInt& aValue)
{
	if(aValue == 0) 			// end points are not used
	{ 
		aValue = 1; 
	}
	else if (aValue == 128) // not used
	{ 
		aValue = 255; 
	}  
	else if (aValue >= 255) // end points are not used
	{ 
		aValue = 254; 
	}
}

// -----------------------------------------------------------------------------
// CMovieProcessorImpl::GetOutputVideoMimeType
// Return Mime type for output video codec
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
TPtrC8& CMovieProcessorImpl::GetOutputVideoMimeType()
{
    VPASSERT(iMovie);
 
    return iMovie->VideoCodecMimeType();    
}

// -----------------------------------------------------------------------------
// CMovieProcessorImpl::GetOutputAVCLevel
// Get output AVC level
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
TInt CMovieProcessorImpl::GetOutputAVCLevel()
{

    VPASSERT( iOutputVideoType == EVideoAVCProfileBaseline );
 
    const TPtrC8& mimeType = iMovie->VideoCodecMimeType();

    if ( mimeType.MatchF( _L8("*profile-level-id=42800A*") ) != KErrNotFound )
    {
        // baseline profile level 1    	
        return 10;
    }      
                      
    else if ( mimeType.MatchF( _L8("*profile-level-id=42900B*") ) != KErrNotFound )
    {
        // baseline profile level 1b    	
        return 101;  // internal constant for level 1b
    }
        
    else if ( mimeType.MatchF( _L8("*profile-level-id=42800B*") ) != KErrNotFound )
    {
        // baseline profile level 1.1    
        return 11;
    }
        
    else if ( mimeType.MatchF( _L8("*profile-level-id=42800C*") ) != KErrNotFound )
    {
        // baseline profile level 1.2    
        return 12;
    }
    //WVGA task
    else if ( mimeType.MatchF( _L8("*profile-level-id=42800D*") ) != KErrNotFound )
    {
        // baseline profile level 1.3    
        return 13;
    } 
    else if ( mimeType.MatchF( _L8("*profile-level-id=428014*") ) != KErrNotFound )
    {
        // baseline profile level 2    
        return 20;
    } 
    else if ( mimeType.MatchF( _L8("*profile-level-id=428015*") ) != KErrNotFound )
    {
        // baseline profile level 2.1    
        return 21;
    } 
    else if ( mimeType.MatchF( _L8("*profile-level-id=428016*") ) != KErrNotFound )
    {
        // baseline profile level 2.2    
        return 22;
    }
    else if ( mimeType.MatchF( _L8("*profile-level-id=42801E*") ) != KErrNotFound )
    {
        // baseline profile level 3    
        return 30;
    } 
    else if ( mimeType.MatchF( _L8("*profile-level-id=42801F*") ) != KErrNotFound )
    {
        // baseline profile level 3.1    
        return 31;
    } 
    
    else if ( mimeType.MatchF( _L8("*profile-level-id=*") ) != KErrNotFound )
    {
        // no other profile-level ids supported
        User::Panic(_L("CMovieProcessorImpl"), EInvalidInternalState);        
    }
    
    else
    {
        // Default is level 1 (?)
        return 10;
    }
    return 10;
}

// -----------------------------------------------------------------------------
// CMovieProcessorImpl::SuspendProcessing
// Suspends processing
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
TInt CMovieProcessorImpl::SuspendProcessing()
{

    PRINT((_L("CMovieProcessorImpl::SuspendProcessing()")));

    iDemux->Stop();
    
    return KErrNone;
}


// -----------------------------------------------------------------------------
// CMovieProcessorImpl::ResumeProcessing
// Resumes processing
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
TInt CMovieProcessorImpl::ResumeProcessing(TInt& aStartFrameIndex, TInt aFrameNumber)
{

    PRINT((_L("CMovieProcessorImpl::ResumeProcessing(), frame number = %d"), aFrameNumber));
    PRINT((_L("CMovieProcessorImpl::ResumeProcessing(), start index = %d"),  iStartFrameIndex ));

    // get index of last written frame
    TInt index = iStartFrameIndex + aFrameNumber;

    PRINT((_L("CMovieProcessorImpl::ResumeProcessing(), index is %d"), index));

    TInt ticks;    
    // get start time for next frame
    TInt time = iParser->GetVideoFrameStartTime(index + 1, &ticks);
    if ( time < 0 )
        {
        // time represents an error code from parser
        PRINT((_L("CMovieProcessorImpl::ResumeProcessing(), error from iParser %d"), time));
        return time;
        }

    PRINT((_L("CMovieProcessorImpl::ResumeProcessing(), start frame time = %d ms"), time));
    iStartCutTime =	TTimeIntervalMicroSeconds(TInt64(time) * TInt64(1000));    
    
    // reset parser variables 
    iParser->Reset();        
 
    // seek to Intra from where to start decoding to resume processing
    TInt error = iParser->SeekOptimalIntraFrame(iStartCutTime, 0, EFalse);
    
    if (error != KErrNone)
       return error;
    
    aStartFrameIndex = iParser->GetStartFrameIndex();
    
    PRINT((_L("CMovieProcessorImpl::ResumeProcessing(), aStartFrameIndex = %d"), aStartFrameIndex));
    
    // iStartFrameIndex contains the index of the first included
    // frame, which is != 0 in case the clip is cut from beginning        
    aStartFrameIndex -= iStartFrameIndex;
    
    PRINT((_L("CMovieProcessorImpl::ResumeProcessing(), aStartFrameIndex = %d"), aStartFrameIndex));        
        
    iVideoQueue->ResetStreamEnd();
    iDemux->Start();
    
    return KErrNone;   
}

// -----------------------------------------------------------------------------
// CMovieProcessorImpl::NeedTranscoderAnyMore
// Check if all video is processed already
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
TBool CMovieProcessorImpl::NeedTranscoderAnyMore()
    {
    PRINT((_L("CMovieProcessorImpl::NeedTranscoderAnyMore()")));
    if ( iAllVideoProcessed )
        {
        PRINT((_L("CMovieProcessorImpl::NeedTranscoderAnyMore() EFalse")));
        return EFalse;
        }
    else
        {
        PRINT((_L("CMovieProcessorImpl::NeedTranscoderAnyMore() ETrue")));
        return ETrue;
        }
        
    
    }
    

//  OTHER EXPORTED FUNCTIONS


//=============================================================================


//  End of File

