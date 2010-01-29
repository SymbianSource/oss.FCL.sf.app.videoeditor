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


// System includes
#include <gulicon.h>
#include <coemain.h>
#include <eikenv.h>
#include <akniconutils.h>
#include <aknsdrawutils.h> 
#include <aknsdatacontext.h> 
#include <akniconutils.h>
#include <aknbitmapanimation.h>
#include <barsread.h>
#include <VideoEditorUiComponents.mbg>
#include <VideoEditorUiComponents.rsg>
#include <audiopreference.h>
#include <mmf/common/mmfcontrollerpluginresolver.h>
#include <ConeResLoader.h>
#include <data_caging_path_literals.hrh>
#include <mmf/common/mmfcontrollerframeworkbase.h> 
#include <mmf/common/mmferrors.h>
#ifdef VERBOSE
#include <eikappui.h>
#endif

// User includes
#include "VeiVideoEditorSettings.h"
#include "VeiVideoDisplay.h"
#include "VideoEditorCommon.h"
#include "VideoEditorUtils.h"
#include "VideoEditorDebugUtils.h"

// CONSTANTS
_LIT(KResourceFile, "VideoEditorUiComponents.rsc");

EXPORT_C CVeiVideoDisplay* CVeiVideoDisplay::NewL( const TRect& aRect, const CCoeControl* aParent,
										  MVeiVideoDisplayObserver& aObserver )
    {
    CVeiVideoDisplay* self = CVeiVideoDisplay::NewLC( aRect, aParent, aObserver );
    CleanupStack::Pop(self);
    return self;
    }

EXPORT_C CVeiVideoDisplay* CVeiVideoDisplay::NewLC( const TRect& aRect, const CCoeControl* aParent,
										   MVeiVideoDisplayObserver& aObserver )
    {
    CVeiVideoDisplay* self = new (ELeave) CVeiVideoDisplay (aObserver);
    CleanupStack::PushL( self );
    self->ConstructL( aRect, aParent );
    return self;
    }

CVeiVideoDisplay::CVeiVideoDisplay( MVeiVideoDisplayObserver& aObserver ) : iObserver (aObserver)
	{
	}	

void CVeiVideoDisplay::ConstructL( const TRect& aRect, 
                                   const CCoeControl* aParent)
    {
    LOG(KVideoEditorLogFile, "CVeiVideoDisplay::ConstructL: in");

	TFileName mbmPath( VideoEditorUtils::IconFileNameAndPath(KVideoEditorUiComponentsIconFileId) );
    
    
// Backgroud squares icon loading is disabled there is no suitable graphic in 
// S60 build at the moment. A new graphic should be requested and added here.
    
//	AknIconUtils::CreateIconL( iBgSquaresBitmap, iBgSquaresBitmapMask,
//			mbmPath, EMbmVideoeditoruicomponentsQgn_graf_ve_novideo, 
//			EMbmVideoeditoruicomponentsQgn_graf_ve_novideo_mask );    
    
    
	TRAP_IGNORE( CVeiVideoEditorSettings::GetMediaPlayerVolumeLevelL( iInternalVolume ) );

	iMuted = (iInternalVolume == 0);

	iBorderWidth = 2;
	iDuration = 0;
	iMaxVolume = 0;
	iBlank = ETrue;
	iAnimationOn = EFalse;
	iBufferingCompleted = ETrue;
	SetContainerWindowL( *aParent );

	SetRect( aRect );
    ActivateL();

	ControlEnv()->AddForegroundObserverL( *this );

    LOG(KVideoEditorLogFile, "CVeiVideoDisplay::ConstructL: out");
    }

void CVeiVideoDisplay::SizeChanged()
    {
	LOGFMT2(KVideoEditorLogFile, "CVeiVideoDisplay::SizeChanged: in: Size(): (%d,%d)", Size().iWidth, Size().iHeight);

	if ( iVideoPlayerUtility )
		{
		TRect screenRect = CalculateVideoPlayerArea();
		LOGFMT4(KVideoEditorLogFile, "CVeiVideoDisplay::SizeChanged 1, ScreenRect Tl:(%d,%d) Br:(%d,%d)", screenRect.iTl.iX,screenRect.iTl.iY,screenRect.iBr.iX,screenRect.iBr.iY);

		/* Get the required parameters for the video player. */
		CCoeEnv* coeEnv = CCoeEnv::Static();
		RWsSession& session = coeEnv->WsSession();
		CWsScreenDevice* screenDevice = coeEnv->ScreenDevice();
		RDrawableWindow* drawableWindow = DrawableWindow();

		TRAPD(err, iVideoPlayerUtility->SetDisplayWindowL( session, *screenDevice, *drawableWindow,
			screenRect, screenRect ) );
		if( KErrNone != err )
			{
			LOGFMT(KVideoEditorLogFile, "CVeiVideoDisplay::SizeChanged: iVideoPlayerUtility->SetDisplayWindowL failed: %d", err);
			iObserver.NotifyVideoDisplayEvent( MVeiVideoDisplayObserver::EError );
			}
		}
	AknIconUtils::SetSize( iBgSquaresBitmap, Size(), EAspectRatioNotPreserved );


	if( iAnimationOn )
		{
		iAnimation->CancelAnimation();
		TRect rect( Rect() );
		rect.Move( iBorderWidth, iBorderWidth );
		rect.Resize( -iBorderWidth*2, -iBorderWidth );

		iAnimation->SetRect( rect );
		TRAPD(err, iAnimation->StartAnimationL() );
		if( KErrNone != err )
			{
			LOGFMT(KVideoEditorLogFile, "CVeiVideoDisplay::SizeChanged: StartAnimationL failed: %d", err);
			iObserver.NotifyVideoDisplayEvent( MVeiVideoDisplayObserver::EError );
			}
		}

	LOG(KVideoEditorLogFile, "CVeiVideoDisplay::SizeChanged: out");
	}

TRect CVeiVideoDisplay::CalculateVideoPlayerArea()
	{
	/* Calculate regions. */ 
	TRect screenRect;
	TRect rect( Rect() );
	/* In full screen mode return whole screen rect*/
	if ( rect.iTl == TPoint( 0,0) )
		{
		return rect;
		}
	LOGFMT4(KVideoEditorLogFile, "CVeiVideoDisplay::CalculateVideoPlayerArea() 1: Rect() Tl:(%d,%d) Br:(%d,%d)", rect.iTl.iX,rect.iTl.iY,rect.iBr.iX,rect.iBr.iY);
	
	TPoint position = PositionRelativeToScreen();
	LOGFMT2(KVideoEditorLogFile, "CVeiVideoDisplay::CalculateVideoPlayerArea() 2: PositionRelativeToScreen(%d,%d)", position.iX, position.iY);

	screenRect = rect;  
	screenRect.Move( TPoint( iBorderWidth, (position.iY-Rect().iTl.iY) + iBorderWidth ) ); 
	screenRect.Resize( -(iBorderWidth*2), -(iBorderWidth*2 ));
	return screenRect;
	}

TKeyResponse CVeiVideoDisplay::OfferKeyEventL(const TKeyEvent& aKeyEvent,TEventCode aType)
	{
	LOGFMT2(KVideoEditorLogFile, "CVeiVideoDisplay::OfferKeyEventL: aKeyEvent.iCode:%d, aType:%d", aKeyEvent.iCode, aType);

	if ( iMuted )
		{
		LOG(KVideoEditorLogFile, "CVeiVideoDisplay::OfferKeyEventL: volume is muted.");
		return EKeyWasNotConsumed;
		}
	
	if ( aType == EEventKey )
		{
		switch (aKeyEvent.iCode)
			{
			case EKeyDownArrow:
			case EStdKeyDecVolume:
				{
				AdjustVolumeL( -1 );
				return EKeyWasConsumed;
				}
			case EKeyUpArrow:
			case EStdKeyIncVolume:
				{
				AdjustVolumeL( 1 );
				return EKeyWasConsumed;
				}
			default:
				{
				return EKeyWasConsumed;
				}
			}
		}
	return EKeyWasConsumed;
	}

EXPORT_C CVeiVideoDisplay::~CVeiVideoDisplay()
	{
	LOG(KVideoEditorLogFile, "CVeiVideoDisplay::~CVeiVideoDisplay(): In");
	// Remove foreground event observer
	ControlEnv()->RemoveForegroundObserver( *this );
	if ( iDisplayBitmap )
		{
		delete iDisplayBitmap;
		iDisplayBitmap = NULL;
		}
	if ( iDisplayMask )
		{
		delete iDisplayMask;
		iDisplayMask = NULL;
		}
	delete iBgSquaresBitmap;
	delete iBgSquaresBitmapMask;

	if ( iVideoPlayerUtility != NULL )
		{
		iVideoPlayerUtility->Close();
		delete iVideoPlayerUtility;
		iVideoPlayerUtility = NULL;
		}
	delete iAnimation;
	delete iCallBack;
	delete iFilename;
	LOG(KVideoEditorLogFile, "CVeiVideoDisplay::~CVeiVideoDisplay(): Out");
	}

EXPORT_C void CVeiVideoDisplay::ShowPictureL( const CFbsBitmap& aBitmap, const CFbsBitmap& aMask )
	{
	LOG(KVideoEditorLogFile, "CVeiVideoDisplay::ShowPictureL: In");

	StoreDisplayBitmapL( aBitmap, &aMask );

	DrawDeferred();
	LOG(KVideoEditorLogFile, "CVeiVideoDisplay::ShowPictureL: Out");
	}

EXPORT_C void CVeiVideoDisplay::ShowPictureL( const CFbsBitmap& aBitmap )
	{
	LOG(KVideoEditorLogFile, "CVeiVideoDisplay::ShowPicture2: In");

	StoreDisplayBitmapL( aBitmap );

	// set screen size to pause mode
	if ( iVideoPlayerUtility )
		{
		TRect screenRect = TRect( TPoint(0,0), TPoint(0,0) );			
		LOGFMT4(KVideoEditorLogFile, "CVeiVideoDisplay::SizeChanged 1, ScreenRect Tl:(%d,%d) Br:(%d,%d)", screenRect.iTl.iX,screenRect.iTl.iY,screenRect.iBr.iX,screenRect.iBr.iY);

		/* Get the required parameters for the video player. */
		CCoeEnv* coeEnv = CCoeEnv::Static();
		RWsSession& session = coeEnv->WsSession();
		CWsScreenDevice* screenDevice = coeEnv->ScreenDevice();
		RDrawableWindow* drawableWindow = DrawableWindow();

		iVideoPlayerUtility->SetDisplayWindowL( session, *screenDevice, *drawableWindow,
			screenRect, screenRect );
		}

	DrawDeferred();
	LOG(KVideoEditorLogFile, "CVeiVideoDisplay::ShowPicture2: Out");
	}

EXPORT_C void CVeiVideoDisplay::SetPictureL( const CFbsBitmap& aBitmap )
	{
	LOG(KVideoEditorLogFile, "CVeiVideoDisplay::SetPictureL: In");

	StoreDisplayBitmapL( aBitmap );

	LOG(KVideoEditorLogFile, "CVeiVideoDisplay::SetPictureL: Out");	
	}

void CVeiVideoDisplay::StoreDisplayBitmapL( const CFbsBitmap& aBitmap, const CFbsBitmap* aMask )
	{
	LOG(KVideoEditorLogFile, "CVeiVideoDisplay::StoreDisplayBitmapL: In");

	iBlank = EFalse;
	iBlack = EFalse;

	// Delete old bitmaps
	if ( iDisplayBitmap )
		{
		delete iDisplayBitmap;
		iDisplayBitmap = NULL;
		}
	if ( iDisplayMask )
		{
		delete iDisplayMask;
		iDisplayMask = NULL;
		}

	// Create new bitmaps
	iDisplayBitmap = new (ELeave) CFbsBitmap;
	iDisplayBitmap->Duplicate( aBitmap.Handle() );
	if (aMask)
		{
		iDisplayMask = new (ELeave) CFbsBitmap;
		iDisplayMask->Duplicate( aMask->Handle() );
		}

#ifdef VERBOSE
	// Write the display bitmap to disk
	_LIT(KFileName, "C:\\data\\images\\videoeditor\\iDisplayBitmap.mbm");
	TFileName saveToFile( KFileName );
	CEikonEnv* env = CEikonEnv::Static();
	env->EikAppUi()->Application()->GenerateFileName( env->FsSession(), saveToFile );
	User::LeaveIfError( iDisplayBitmap->Save( saveToFile ) );
	LOGFMT(KVideoEditorLogFile, "CVeiVideoDisplay::StoreDisplayBitmapL: saved %S", &saveToFile);
#endif
	LOG(KVideoEditorLogFile, "CVeiVideoDisplay::StoreDisplayBitmapL: Out");
	}

EXPORT_C void CVeiVideoDisplay::ShowBlackScreen()
	{
	LOG(KVideoEditorLogFile, "CVeiVideoDisplay::ShowBlackScreen: In");	
	iBlack = ETrue;
	DrawDeferred();
	LOG(KVideoEditorLogFile, "CVeiVideoDisplay::ShowBlackScreen: Out");	
	}
EXPORT_C void CVeiVideoDisplay::SetBlackScreen( TBool aBlack )
    {
    LOGFMT(KVideoEditorLogFile, "CVeiVideoDisplay::SetBlackScreen: In and out, aBlack:%d", aBlack);	
    iBlack = aBlack;
    }
EXPORT_C void CVeiVideoDisplay::ShowBlankScreen()
	{
	LOG(KVideoEditorLogFile, "CVeiVideoDisplay::ShowBlankScreen: In and out");	
	iBlank = ETrue;
	DrawDeferred();
	}

EXPORT_C TTimeIntervalMicroSeconds CVeiVideoDisplay::PositionL() const
	{
	if ( !iVideoPlayerUtility )
		return TTimeIntervalMicroSeconds(0);
	else
		return iVideoPlayerUtility->PositionL();
	}

EXPORT_C TVideoRotation CVeiVideoDisplay::RotationL() const
    {
    if ( iVideoPlayerUtility )
    	{
        return iVideoPlayerUtility->RotationL();
    	}
    else
    	{
        return EVideoRotationNone;
    	}
    }

EXPORT_C void CVeiVideoDisplay::SetPositionL( const TTimeIntervalMicroSeconds& aPosition )
	{
	LOGFMT(KVideoEditorLogFile, "VideoDisplay SetPositionL: %Ld", aPosition.Int64());
	iStartPoint = aPosition;
	if ( iVideoPlayerUtility && iBufferingCompleted )
		{	
		iVideoPlayerUtility->SetPositionL( iStartPoint );
		}
	}

EXPORT_C void CVeiVideoDisplay::SetRotationL(TVideoRotation aRotation)
    {
	LOG(KVideoEditorLogFile, "CVeiVideoDisplay::SetRotationL: in");
    if ( iVideoPlayerUtility )
		{
		iVideoPlayerUtility->SetRotationL( aRotation );
		}
	LOG(KVideoEditorLogFile, "CVeiVideoDisplay::SetRotationL: out");
    }


EXPORT_C void CVeiVideoDisplay::Stop( TBool aCloseStream )
	{
	LOG(KVideoEditorLogFile, "CVeiVideoDisplay::Stop: in");	
	if ( iVideoPlayerUtility )
		{
		// this is poor solution, responsibility of taking care of iStartPoint be elsewhere
		TRAP_IGNORE(iStartPoint = PositionL());
		iVideoPlayerUtility->Stop();
	
		if ( aCloseStream )
			{
			LOG(KVideoEditorLogFile, "CVeiVideoDisplay::Stop 1, Close CVideoPlayerUtility.");
			iVideoPlayerUtility->Close(); 
			delete iVideoPlayerUtility;
			iVideoPlayerUtility = NULL;
			}
		}
	LOG(KVideoEditorLogFile, "CVeiVideoDisplay::Stop 2");
	iObserver.NotifyVideoDisplayEvent( MVeiVideoDisplayObserver::EStop );

	LOG(KVideoEditorLogFile, "CVeiVideoDisplay::Stop: out");
	}

EXPORT_C TInt CVeiVideoDisplay::GetBorderWidth() const
	{
	return iBorderWidth;
	}

EXPORT_C TSize CVeiVideoDisplay::GetScreenSize() const
	{
	TRect rect( Rect() );
	rect.Shrink( iBorderWidth, iBorderWidth-1 );
	return rect.Size();
	}

EXPORT_C void CVeiVideoDisplay::Play()
	{
	/*LOG(KVideoEditorLogFile, "CVeiVideoDisplay::Play :in");
	if (iVideoPlayerUtility &&  iBufferingCompleted )
		{	
		LOG(KVideoEditorLogFile, "CVeiVideoDisplay::Play :2");
		//SizeChanged();
		iVideoPlayerUtility->Play();
		}
	LOG(KVideoEditorLogFile, "CVeiVideoDisplay::Play: out");
	*/
	PlayL(*iFilename);
	}

EXPORT_C void CVeiVideoDisplay::PlayMarkedL( const TTimeIntervalMicroSeconds& aStartPoint, const TTimeIntervalMicroSeconds& aEndPoint)
	{
	/*
	LOGFMT2(KVideoEditorLogFile, "CVeiVideoDisplay::PlayMarked:In, aStartPoint:%Ld, aEndPoint:%Ld", aStartPoint.Int64(), aEndPoint.Int64());
	iVideoPlayerUtility->SetPositionL(aStartPoint);
	//SizeChanged();
	iVideoPlayerUtility->Play(aStartPoint, aEndPoint);
	LOG(KVideoEditorLogFile, "CVeiVideoDisplay::PlayMarked :Out");
	*/
	PlayL(*iFilename, aStartPoint, aEndPoint);
	}

EXPORT_C void CVeiVideoDisplay::PlayL( 	const TDesC& aFilename,
								const TTimeIntervalMicroSeconds& aStartPoint, 
								const TTimeIntervalMicroSeconds& aEndPoint )
	{
	LOGFMT3(KVideoEditorLogFile, "CVeiVideoDisplay::PlayL(): In, aFilename:%S, aStartPoint:%Ld, aEndPoint:%Ld", &aFilename, aStartPoint.Int64(), aEndPoint.Int64());
		
	if ( iVideoPlayerUtility && iBufferingCompleted)
		{
		LOG(KVideoEditorLogFile, "CVeiVideoDisplay::PlayL 1");
		if ( aEndPoint.Int64() != 0 )
			{
			LOGFMT2(KVideoEditorLogFile, "CVeiVideoDisplay::Play 2, iStartPoint:%Ld, aStartPoint:%Ld", iStartPoint.Int64(), aStartPoint.Int64());	
			iStartPoint = aStartPoint;
			//iVideoPlayerUtility->SetPositionL(iStartPoint);
			//SizeChanged();
			iVideoPlayerUtility->Stop();
			iVideoPlayerUtility->Play( aStartPoint, aEndPoint );
			}
		else
			{
			LOG(KVideoEditorLogFile, "CVeiVideoDisplay::PlayL 3");	
			//SizeChanged();
			//iVideoPlayerUtility->Play(iStartPoint, iVideoPlayerUtility->DurationL());
			iVideoPlayerUtility->Play();
			}
		}
	else if (!iVideoPlayerUtility && iBufferingCompleted) /* should this be: if (! iVideoPlayerUtility)*/
		{		
		LOG(KVideoEditorLogFile, "CVeiVideoDisplay::PlayL 4");
		if ( aEndPoint.Int64() != 0 )
			{
			iStartPoint = aStartPoint;
			iEndPoint = aEndPoint;	
			}	
		OpenFileL( aFilename );
		}
	LOG(KVideoEditorLogFile, "CVeiVideoDisplay::PlayL(): out");
	}

EXPORT_C void CVeiVideoDisplay::PauseL()
	{
	LOG(KVideoEditorLogFile, "CVeiVideoDisplay::PauseL(): in");
	if ( iVideoPlayerUtility && iBufferingCompleted )
		{
		LOG(KVideoEditorLogFile, "CVeiVideoDisplay::PauseL 1, calling pause");
		iVideoPlayerUtility->PauseL();
		
		/*						
		//@: for some reason this does not work 
		TDisplayMode dmode = EColor64K;		
		iVideoPlayerUtility->GetFrameL(dmode, ContentAccess::EPlay);
		*/
		
		/*
		//@: do not work either:
		LOG(KVideoEditorLogFile, "CVeiVideoDisplay::PauseL(): 2, refreshing");
		TInt err = KErrNone;
		TRAP(err, iVideoPlayerUtility->RefreshFrameL());
		LOGFMT(KVideoEditorLogFile, "CVeiVideoDisplay::PauseL(): 3, err:%d", err);
		*/
		}
	LOG(KVideoEditorLogFile, "CVeiVideoDisplay::PauseL(): out");
	}

EXPORT_C void CVeiVideoDisplay::OpenFileL( const TDesC& aFilename )
    {
	LOGFMT(KVideoEditorLogFile, "CVeiVideoDisplay::OpenFileL: In, aFilename:%S", &aFilename);		

	/* Calculate regions. */ 
    TRect screenRect = CalculateVideoPlayerArea();

	/* Get the required parameters for the video player. */
	CCoeEnv* coeEnv = CCoeEnv::Static();
	RWsSession& session = coeEnv->WsSession();
	CWsScreenDevice* screenDevice = coeEnv->ScreenDevice();
	RDrawableWindow* drawableWindow = DrawableWindow();

	if ( iVideoPlayerUtility != NULL )
		{
		delete iVideoPlayerUtility;
		iVideoPlayerUtility = NULL;		
		}
		
	if (!iFilename)
		{	
		iNewFile = ETrue;
		iDuration = TTimeIntervalMicroSeconds(0);
		iFilename = aFilename.AllocL();		
		}	
	else if ((*iFilename).CompareF(aFilename))
		{
		iNewFile = ETrue;
		iDuration = TTimeIntervalMicroSeconds(0);
		HBufC* temp = aFilename.AllocL();
		delete iFilename;
		iFilename = temp;
		}

	/* Initialize the video player. */
	iVideoPlayerUtility = CVideoPlayerUtility::NewL( *this, 
	static_cast<enum TMdaPriority>(KAudioPriorityRealOnePlayer),
	static_cast<enum TMdaPriorityPreference>(KAudioPrefRealOneLocalPlayback),
	session, *screenDevice, *drawableWindow, screenRect, screenRect );

	LOG(KVideoEditorLogFile, "CVeiVideoDisplay::OpenFileL 2");
	iObserver.NotifyVideoDisplayEvent( MVeiVideoDisplayObserver::ELoadingStarted );
	iVideoPlayerUtility->RegisterForVideoLoadingNotification( *this );
	/* Open the file. */	
	iVideoPlayerUtility->OpenFileL( *iFilename );

	LOG(KVideoEditorLogFile, "CVeiVideoDisplay::OpenFileL: Out");
    }
// ---------------------------------------------------------
// CVeiPreviewContainer::MvpuoFrameReady( CFbsBitmap& aFrame, TInt aError )
// Notification to the client that the frame requested by a call to GetFrameL is ready.
// ---------------------------------------------------------
void CVeiVideoDisplay::MvpuoFrameReady( CFbsBitmap& /*aFrame*/, TInt DEBUGLOG_ARG(aError) )
	{	
	LOGFMT(KVideoEditorLogFile, "CVeiVideoDisplay::MvpuoFrameReady: In and Out, aError:%d", aError);
	/*
	@: for some reason this does not work 
	if (KErrNone == aError)
		{
		SetPictureL(aFrame);		
		}
	DrawDeferred();
	*/			
	}

// ---------------------------------------------------------
// CVeiVideoDisplay::MvpuoOpenComplete( TInt aError )
// Notification to the client that the opening of the video clip has completed.
// ---------------------------------------------------------
void CVeiVideoDisplay::MvpuoOpenComplete( TInt aError )
	{
	LOGFMT(KVideoEditorLogFile, "CVeiVideoDisplay::MvpuoOpenComplete: In, aError:%d", aError);
	if( aError != KErrNone )
		{
		iObserver.NotifyVideoDisplayEvent( MVeiVideoDisplayObserver::EError );
		return;
		}
	
	iVideoPlayerUtility->Prepare();
	LOG(KVideoEditorLogFile, "CVeiVideoDisplay::MvpuoOpenComplete: Out");
	}

// ---------------------------------------------------------
// CVeiVideoDisplay::MvpuoPlayComplete( TInt aError )
// Notification that video playback has completed.
// ---------------------------------------------------------
void CVeiVideoDisplay::MvpuoPlayComplete( TInt aError )
	{
	LOGFMT(KVideoEditorLogFile, "CVeiVideoDisplay::MvpuoPlayComplete:In, aError:%d", aError);
	iStartPoint = TTimeIntervalMicroSeconds(0);
	/* iBufferingCompleted is set to EFalse in MvloLoadingStarted and set back ETrue in 
	   MvloLoadingCompleted.
	   If error occurs in the middle of the loading process the latter one is not called and 
	   iBufferingCompleted is left to EFalse, that is why is it set to ETrue here also for safety's sake
	*/   
	iBufferingCompleted = ETrue;
	iObserver.NotifyVideoDisplayEvent( MVeiVideoDisplayObserver::EPlayComplete, aError );			
	
	// Stop() better to be called from iObserver 
	/*if ( KErrSessionClosed == aError ) // -45 (10/2006)
		{		
		Stop( ETrue );
		}	
		*/

	LOG(KVideoEditorLogFile, "CVeiVideoDisplay::MvpuoPlayComplete: Out");
	}

// ---------------------------------------------------------
// CVeiPreviewContainer::MvpuoEvent( const TMMFEvent& /*aEvent*/ )
// 
// ---------------------------------------------------------
void CVeiVideoDisplay::MvpuoEvent( const TMMFEvent& aEvent )
	{	
	LOGFMT2(KVideoEditorLogFile, "CVeiVideoDisplay::MvpuoEvent In, aEvent.iEventType:%d, aEvent.iErrorCode:%d",
		aEvent.iEventType.iUid, aEvent.iErrorCode);			
	
	if (KMMFEventCategoryVideoPlayerGeneralError == aEvent.iEventType  &&
		KErrMMAudioDevice == aEvent.iErrorCode)
		{
		iObserver.NotifyVideoDisplayEvent( MVeiVideoDisplayObserver::EError, aEvent.iErrorCode );	
		}
	
	LOG(KVideoEditorLogFile, "CVeiVideoDisplay::MvpuoEvent, Out");	
	}

// ---------------------------------------------------------
// CVeiVideoDisplay::MvpuoPrepareComplete( TInt aError )
// Notification to the client that the opening of the video clip has been prepared.
// ---------------------------------------------------------
void CVeiVideoDisplay::MvpuoPrepareComplete( TInt aError )
	{
	LOGFMT(KVideoEditorLogFile, "CVeiVideoDisplay::MvpuoPrepareComplete:In, aError:%d",aError);
		
	//LocateEntryL();
	if( KErrNone != aError )
		{		
		iObserver.NotifyVideoDisplayEvent( MVeiVideoDisplayObserver::EError, aError );
		return;
		}

	SetPlaybackVolumeL();
 
	SetPositionL(iStartPoint);
	iObserver.NotifyVideoDisplayEvent( MVeiVideoDisplayObserver::EOpenComplete, iStartPoint.Int64() );
/** If volume is 0(muted) give event so muted icon is drawn to navipane */
	if ( iInternalVolume == 0 )
		{
		LOG(KVideoEditorLogFile, "CVeiVideoDisplay::MvpuoPrepareComplete 1");
		iObserver.NotifyVideoDisplayEvent( MVeiVideoDisplayObserver::EVolumeLevelChanged);
		}
	LOG(KVideoEditorLogFile, "CVeiVideoDisplay::MvpuoPrepareComplete: Out");	
	}
	

void CVeiVideoDisplay::LocateEntryL()
	{
	TInt metaDataCount = iVideoPlayerUtility->NumberOfMetaDataEntriesL();
	CMMFMetaDataEntry* entry = NULL;
	// Loop through metadata
	for ( TInt i = 0; i < metaDataCount; i++ )
		{
		entry =	iVideoPlayerUtility->MetaDataEntryL( i );		
		HBufC* name = entry->Name().AllocLC();
		HBufC* value = entry->Value().AllocLC();
		LOGFMT3(KVideoEditorLogFile, "CVeiVideoDisplay::LocateEntryL, i:%d, name,value:%S,%S", i, name, value);
		CleanupStack::PopAndDestroy( value );
		CleanupStack::PopAndDestroy( name );
		}		
	}

void CVeiVideoDisplay::MvloLoadingStarted()
	{
	LOG(KVideoEditorLogFile, "CVeiVideoDisplay::MvloLoadingStarted, In");
	iBufferingCompleted = EFalse;
	iObserver.NotifyVideoDisplayEvent( MVeiVideoDisplayObserver::EBufferingStarted );
	LOG(KVideoEditorLogFile, "CVeiVideoDisplay::MvloLoadingStarted, Out");
	}

void CVeiVideoDisplay::MvloLoadingComplete()
	{
	LOGFMT(KVideoEditorLogFile, "CVeiVideoDisplay::MvloLoadingComplete, In, iStartPoint:%Ld", iStartPoint.Int64());
	iBufferingCompleted = ETrue;			
	iObserver.NotifyVideoDisplayEvent( MVeiVideoDisplayObserver::ELoadingComplete, iStartPoint.Int64() );
	// set screen size to play mode
	SizeChanged();	
	LOG(KVideoEditorLogFile, "CVeiVideoDisplay::MvloLoadingComplete, Out");
	}

void CVeiVideoDisplay::Draw( const TRect& aRect ) const
	{
	LOG(KVideoEditorLogFile, "CVeiVideoDisplay::Draw in");
    CWindowGc& gc = SystemGc();

	TRect rect( Rect() );

	rect.Move(iBorderWidth,iBorderWidth);
	rect.Resize(-iBorderWidth*2,-iBorderWidth*2);
	TSize clipRect( rect.Size() );

    if ( Window().DisplayMode() == EColor16MA )
        {
        gc.SetDrawMode(CGraphicsContext::EDrawModeWriteAlpha);
        gc.SetBrushColor(TRgb::Color16MA( 0 ));
        gc.Clear(aRect);
        }
	
   	if ( iBlack )
   		{        
/*   		
   		LOG(KVideoEditorLogFile, "CVeiVideoDisplay::Draw 1");
        gc.SetPenStyle( CWindowGc::ENullPen );
		gc.SetBrushColor( KRgbBlack );
		gc.SetBrushStyle( CGraphicsContext::ESolidBrush );
		gc.DrawRect( rect );

        gc.SetPenStyle( CWindowGc::ESolidPen );
        gc.SetPenSize( TSize(iBorderWidth,iBorderWidth) );
	
	    gc.DrawRoundRect(aRect, TSize(4,4));
*/ 
		return;
        }
	
	if ( iBlank )
		{
		LOG(KVideoEditorLogFile, "CVeiVideoDisplay::Draw 2");
		gc.BitBltMasked( rect.iTl, iBgSquaresBitmap, clipRect, iBgSquaresBitmapMask, EFalse);
		}
	else
		{		
		LOG(KVideoEditorLogFile, "CVeiVideoDisplay::Draw 3");

		if ( iDisplayBitmap->SizeInPixels() == GetScreenSize() )
			{						

			gc.BitBlt( rect.iTl, iDisplayBitmap, clipRect);
			}
		else
			{						

			TSize clipRectSize = iDisplayBitmap->SizeInPixels();                 
            TRect destRect;
            
            // check which one has bigger aspect ratio, video diplay or thumbnail.
            TReal displayAR = rect.Width() / TReal(rect.Height());
            TReal thumbnailAR = clipRectSize.iWidth / TReal(clipRectSize.iHeight);
            
            if (thumbnailAR > displayAR)
                {
                //Create proper destination rect                                    
                TInt newHeight = (clipRectSize.iHeight * rect.Width() ) / clipRectSize.iWidth;
                TInt newTLiY = rect.iTl.iY + (rect.Height() - newHeight) / 2;
                destRect = TRect(TPoint(rect.iTl.iX,newTLiY), TSize(rect.Width(), newHeight));
                }
            else
                {
                // write here destrect calculation when thumbnailAR < displayAR
                TInt newWidth = rect.Height()*thumbnailAR;
                TInt newTLiX = rect.iTl.iX + ((rect.Width()-newWidth)/2);
                destRect = TRect (TPoint(newTLiX, rect.iTl.iY), TSize(newWidth, rect.Height()));
                }
		   gc.DrawBitmap( destRect, iDisplayBitmap );
			}
		}

/*
	gc.SetPenStyle( CWindowGc::ESolidPen );
	gc.SetPenSize( TSize(iBorderWidth,iBorderWidth) );
	
	rect = Rect();
	rect.Resize(-(iBorderWidth-1),-(iBorderWidth-1));
	gc.DrawRoundRect( rect, TSize(4,4));
*/	
	LOG(KVideoEditorLogFile, "CVeiVideoDisplay::Draw out");
	}


EXPORT_C TTimeIntervalMicroSeconds CVeiVideoDisplay::TotalLengthL()
	{
	if ( !iVideoPlayerUtility )
		{
		return iDuration;	
		//return TTimeIntervalMicroSeconds(0);
		}
	else
		{
		TRAPD( err, iDuration =iVideoPlayerUtility->DurationL() );
		if ( err == KErrNone )
			{
			return iDuration;
			}
		else
			{
			return TTimeIntervalMicroSeconds(0);
			}
		}
	}

EXPORT_C void CVeiVideoDisplay::AdjustVolumeL( TInt aIncrement )
	{
	LOGFMT(KVideoEditorLogFile, "CVeiVideoDisplay::AdjustVolumeL( %d ): in", aIncrement);

	iInternalVolume += aIncrement;

	if ( iInternalVolume < 0 )
		{
		iInternalVolume = 0;
		}
	if ( iInternalVolume > KMaxVolumeLevel )
		{
		iInternalVolume = KMaxVolumeLevel;
		}

	SetPlaybackVolumeL();
	iObserver.NotifyVideoDisplayEvent( MVeiVideoDisplayObserver::EVolumeLevelChanged );

	LOG(KVideoEditorLogFile, "CVeiVideoDisplay::SetVolumeL(): out");
	}

void CVeiVideoDisplay::SetPlaybackVolumeL()
	{
	LOGFMT(KVideoEditorLogFile, "CVeiVideoDisplay::SetPlaybackVolumeL(): in: iInternalVolume: %d", iInternalVolume);

	if ( iVideoPlayerUtility )
		{
		// Convert the internal volume to CVideoPlayerUtility's scale.
		iMaxVolume = iVideoPlayerUtility->MaxVolume();

		TInt vol = iMaxVolume * iInternalVolume / KMaxVolumeLevel;
		iVideoPlayerUtility->SetVolumeL( vol );
		}

	LOG(KVideoEditorLogFile, "CVeiVideoDisplay::SetPlaybackVolumeL(): out");
	}

EXPORT_C void CVeiVideoDisplay::ShowAnimationL( TInt aResourceId, TInt aFrameIntervalInMilliSeconds )
	{
	LOG(KVideoEditorLogFile, "CVeiVideoDisplay::ShowAnimationL: in");

	iAnimationResourceId = aResourceId;

	if ( iAnimation )
		{
		delete iAnimation;
		iAnimation = 0;
		}
	/* In slowmotion video thumbnail is shown */
	if ( aResourceId != R_VEI_SLOW_MOTION_ANIMATION )
		{
		iBlack = ETrue;
		DrawNow();
		iBlack = EFalse;
		}
	else
		{
		DrawNow();
		}

	// Locate and open the resource file
    TFileName fileName;
    TParse p;    

    Dll::FileName(fileName);
    p.Set(KResourceFile, &KDC_RESOURCE_FILES_DIR, &fileName);
    fileName = p.FullName();

    LOGFMT(KVideoEditorLogFile, "CVeiVideoDisplay::ShowAnimationL: Loading resource file: %S", &fileName);
    RConeResourceLoader coneResLoader (*iCoeEnv);
	coneResLoader.OpenL( fileName ); // RConeResourceLoader selects the correct language file
	CleanupClosePushL( coneResLoader );

	// Create animation from resource
	iAnimation = CAknBitmapAnimation::NewL();

	TResourceReader reader;
	iCoeEnv->CreateResourceReaderLC( reader, aResourceId );
	iAnimation->ConstructFromResourceL( reader );
	TRect rect( Rect() );
	rect.Move( iBorderWidth, iBorderWidth );
	rect.Resize( -iBorderWidth*2, -iBorderWidth );
	iAnimation->SetRect( rect );
	iAnimation->SetContainerWindowL( *this );
	iAnimationOn = ETrue;
	iAnimation->StartAnimationL();
	CleanupStack::PopAndDestroy(); //reader
	CBitmapAnimClientData* animClientData = iAnimation->BitmapAnimData();
	iAnimationFrameIntervalInMilliSeconds = animClientData->FrameIntervalInMilliSeconds();
	
	CleanupStack::PopAndDestroy(); // coneResLoader

	if ( aFrameIntervalInMilliSeconds > 0 )
		{
		SetFrameIntervalL( aFrameIntervalInMilliSeconds );
		}
	DrawDeferred();

	LOG(KVideoEditorLogFile, "CVeiVideoDisplay::ShowAnimationL: out");
	}

EXPORT_C void CVeiVideoDisplay::StopAnimation()
	{
	if ( iAnimationOn )
		{
		iAnimation->CancelAnimation();
		}
	iAnimationOn = EFalse;
	DrawDeferred();
	}

EXPORT_C void CVeiVideoDisplay::SetFrameIntervalL(TInt aFrameIntervalInMilliSeconds)
	{
	iAnimationFrameIntervalInMilliSeconds+=aFrameIntervalInMilliSeconds;
	iAnimation->SetFrameIntervalL( iAnimationFrameIntervalInMilliSeconds );
	}

void CVeiVideoDisplay::HandleResourceChange(TInt aType)
	{
	if( (aType == KEikMessageFadeAllWindows) && iAnimationOn )
		{
		iAnimation->CancelAnimation();
		}
	else if( (aType == KEikMessageUnfadeWindows) && iAnimationOn )
		{
		TRAPD(err, iAnimation->StartAnimationL() );
		if( KErrNone != err )
			{
			LOGFMT(KVideoEditorLogFile, "CVeiVideoDisplay::HandleResourceChange: StartAnimationL failed: %d", err);
			iObserver.NotifyVideoDisplayEvent( MVeiVideoDisplayObserver::EError );
			}
		}
	}

EXPORT_C TInt CVeiVideoDisplay::Volume() const
	{
	return iInternalVolume;
	}
	
EXPORT_C void CVeiVideoDisplay::SetMuteL( TBool aMuted )
	{
	iMuted = aMuted;	
	}

//=======================================================================================================
void CVeiVideoDisplay::HandleLosingForeground()
	{
	LOG(KVideoEditorLogFile, "CVeiVideoDisplay::HandleLosingForeground(): In");

	if( iAnimationOn )
		{
		LOGFMT(KVideoEditorLogFile, "CVeiVideoDisplay::HandleLosingForeground(): stopping animation. iAnimationFrameIntervalInMilliSeconds == %d", iAnimationFrameIntervalInMilliSeconds);

		// Delete the animation when going to backgroung. This should not be necessary, there are some
		// platforms where the bitmap animation works incorrectly when swithching back (see EFLI-6VL4JS)
		iStoredAnimationFrameIntervalInMilliSeconds = iAnimationFrameIntervalInMilliSeconds;
		delete iAnimation;
		iAnimation = NULL;
		}

	LOG(KVideoEditorLogFile, "CVeiVideoDisplay::HandleLosingForeground(): Out");
	}

//=======================================================================================================
void CVeiVideoDisplay::HandleGainingForeground()
	{
	LOG(KVideoEditorLogFile, "CVeiVideoDisplay::HandleGainingForeground(): In");

	if( iAnimationOn )
		{
		TRAPD(err, ShowAnimationL( iAnimationResourceId, 0 ));
		if( KErrNone == err )
			{
			// restore the animation speed
			LOGFMT2(KVideoEditorLogFile, "CVeiVideoDisplay::HandleLosingForeground(): animation started. next restoring speed to %d (now it is %d)", iStoredAnimationFrameIntervalInMilliSeconds, iAnimationFrameIntervalInMilliSeconds);
			iAnimationFrameIntervalInMilliSeconds = iStoredAnimationFrameIntervalInMilliSeconds;
			// set the animation's frame interval via callback. if set directly after starting the
			// animation, the rate change does not take effect on screen.
			SetAnimationFrameIntervalCallbackL();
			}
		else
			{
			LOGFMT(KVideoEditorLogFile, "CVeiVideoDisplay::SizeChanged: HandleGainingForeground failed: %d", err);
			iObserver.NotifyVideoDisplayEvent( MVeiVideoDisplayObserver::EError );
			}
		}

	LOG(KVideoEditorLogFile, "CVeiVideoDisplay::HandleGainingForeground: Out");
	}

//=======================================================================================================
void CVeiVideoDisplay::SetAnimationFrameIntervalCallbackL()
	{
	LOG( KVideoEditorLogFile, "CVeiVideoDisplay::DoSetAnimationFrameIntervalL: in");

	if (! iCallBack)
		{		
		TCallBack cb (CVeiVideoDisplay::SetAnimationFrameIntervalCallbackMethod, this);
		iCallBack = new (ELeave) CAsyncCallBack(cb, CActive::EPriorityLow);
		}
	iCallBack->CallBack();

	LOG( KVideoEditorLogFile, "CVeiVideoDisplay::DoSetAnimationFrameIntervalL: out");
	}

//=======================================================================================================
TInt CVeiVideoDisplay::SetAnimationFrameIntervalCallbackMethod(TAny* aThis)
	{
    LOG( KVideoEditorLogFile, "CVeiVideoDisplay::SetAnimationFrameIntervalCallbackMethod");

	CVeiVideoDisplay* me = static_cast<CVeiVideoDisplay*>(aThis);
	TRAPD( err, me->SetFrameIntervalL( 0 ) );
	
	return err;
	}

// End of File
