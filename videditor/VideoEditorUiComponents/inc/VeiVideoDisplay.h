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
* Declares VeiCutAudioBar control for the video editor application.
*
*/



#ifndef VEIVIDEODISPLAY_H
#define VEIVIDEODISPLAY_H

#include <coecntrl.h>
#include <coemain.h>
#include <VideoPlayer.h>

const TInt KMaxVolumeLevel = 10;

class CAknBitmapAnimation;
/**
 * Observer for notifying that video player
 * has finished playing
 *
 * 
 */
class MVeiVideoDisplayObserver
	{
public:
		enum TPlayerEvent
		{
        EStop = 0x90, 
		ELoadingStarted,
		EOpenComplete,
        EPlayComplete,
		EBufferingStarted,
		ELoadingComplete,
		EVolumeLevelChanged,
		EError
		};
	/**
	 * Called to notify that amr conversion is completed
	 * 
	 * @param aConvertedFilename   converted filename.
	 * @param aError  <code>KErrNone</code> if conversion was
	 *                completed successfully; one of the system wide
	 *                error codes if conversion failed
	 */
	virtual void NotifyVideoDisplayEvent( const TPlayerEvent aEvent, const TInt& aInfo = 0 ) = 0;
	

	};
/**
 * CVeiVideoDisplay control class.
 */
class CVeiVideoDisplay :	public CCoeControl, 
							public MVideoPlayerUtilityObserver,
							public MVideoLoadingObserver,
							public MCoeForegroundObserver
    {
    public:
        /**
         * Destructor.
         */
        IMPORT_C virtual ~CVeiVideoDisplay();

		/** 
		 * Static factory method.
		 * 
		 * @return  the created CVeiVideoDisplay object
		 */
		IMPORT_C static CVeiVideoDisplay* NewL( const TRect& aRect, const CCoeControl* aParent,
			 MVeiVideoDisplayObserver& aObserver );

		/** 
		 * Static factory method. Leaves the created object in the cleanup
		 * stack.
		 *
		 * @return  the created CVeiCutAudioBar object
		 */
		IMPORT_C static CVeiVideoDisplay* NewLC( const TRect& aRect, const CCoeControl* aParent,
			 MVeiVideoDisplayObserver& aObserver);

	public:

		IMPORT_C TInt Volume() const;
		IMPORT_C void ShowAnimationL( TInt aResourceId, TInt aFrameIntervalInMilliSeconds=-1 );
		IMPORT_C void StopAnimation();
		IMPORT_C void OpenFileL( const TDesC& aFilename );

		IMPORT_C void Play();
		IMPORT_C void PlayL( const TDesC& aFilename,
					const TTimeIntervalMicroSeconds& aStartPoint = 0, 
					const TTimeIntervalMicroSeconds& aEndPoint = 0);
					
		IMPORT_C void PlayMarkedL( const TTimeIntervalMicroSeconds& aStartPoint, 
			const TTimeIntervalMicroSeconds& aEndPoint);

		IMPORT_C void Stop( TBool aCloseStream );

		IMPORT_C void PauseL();

		IMPORT_C void ShowPictureL( const CFbsBitmap& aBitmap, const CFbsBitmap& aMask );

		IMPORT_C void ShowPictureL( const CFbsBitmap& aBitmap );

        IMPORT_C void SetPictureL( const CFbsBitmap& aBitmap );

		IMPORT_C void ShowBlankScreen();

        IMPORT_C void ShowBlackScreen();

        IMPORT_C void SetBlackScreen( TBool aBlack );

		IMPORT_C TTimeIntervalMicroSeconds PositionL() const;

		IMPORT_C void SetPositionL( const TTimeIntervalMicroSeconds& aPosition );

		IMPORT_C TInt GetBorderWidth() const;

		IMPORT_C TSize GetScreenSize() const;

		IMPORT_C TTimeIntervalMicroSeconds TotalLengthL();

		IMPORT_C void SetFrameIntervalL(TInt aFrameIntervalInMilliSeconds);

        IMPORT_C void SetRotationL(TVideoRotation aRotation);

        IMPORT_C TVideoRotation RotationL() const;

		IMPORT_C void SetMuteL( TBool aMuted );

		IMPORT_C void AdjustVolumeL( TInt aIncrement );

	public:
		TKeyResponse OfferKeyEventL(const TKeyEvent& aKeyEvent,TEventCode aType);

	private: // From MCoeForegroundObserver

		virtual void HandleGainingForeground();
		virtual void HandleLosingForeground();

	private:
		TInt TimeIncrement(TInt aKeyCount) const;

		virtual void HandleResourceChange(TInt aType); 
		 /**
         * Default constructor.
		 *
         */
        void ConstructL( const TRect& aRect, const CCoeControl* aParent );
		 /**
         * C++ default constructor.
		 *
         */
        CVeiVideoDisplay( MVeiVideoDisplayObserver& aObserver );
		/*
        * From CoeControl,SizeChanged.
        */
        void SizeChanged();

		// From MVideoPlayerUtilityObserver
		void MvpuoOpenComplete( TInt aError );
		void MvpuoFrameReady( CFbsBitmap& aFrame, TInt aError );
		void MvpuoPlayComplete( TInt aError );
		void MvpuoPrepareComplete( TInt aError );
		void MvpuoEvent( const TMMFEvent& aEvent );
		void MvloLoadingStarted();
		void MvloLoadingComplete();
       /**
        * From CCoeControl,Draw.
		*
		* @param aRect  rectangle to draw
        */
        void Draw(const TRect& aRect) const;

		TRect CalculateVideoPlayerArea();
		
		void LocateEntryL();
		
		void SetPlaybackVolumeL();

		void StoreDisplayBitmapL( const CFbsBitmap& aBitmap, const CFbsBitmap* aMask = NULL);

       /**
        * Set the animation's frame interval via a callback. 
        * This is needed because if the frame interval is altered immediately
        * after the animation is started, the change does not take effect visually.
		* There must be some delay in between.
        */
		void SetAnimationFrameIntervalCallbackL();
		static TInt SetAnimationFrameIntervalCallbackMethod(TAny* aThis);

    private:	// data

		/** Video player utility. */
		CVideoPlayerUtility*	iVideoPlayerUtility;
		CFbsBitmap*				iDisplayBitmap;
		CFbsBitmap*				iDisplayMask;

		CFbsBitmap*				iBgSquaresBitmap;
		CFbsBitmap*				iBgSquaresBitmapMask;

		MVeiVideoDisplayObserver&	iObserver;
		TBool					iBlank;
		TInt					iBorderWidth;
		TTimeIntervalMicroSeconds	iDuration;
		/** Videoplayerutility volume */
		TInt 					iInternalVolume;

		/** Max volume */
		TInt 					iMaxVolume;

		
		TBool					iAnimationOn;
        TBool					iBlack;
		CAknBitmapAnimation*	iAnimation;
		TInt					iAnimationResourceId;
		TInt					iAnimationFrameIntervalInMilliSeconds;
		TInt					iStoredAnimationFrameIntervalInMilliSeconds;

		/** Seek thumbnail position in video clip. */
		//TTimeIntervalMicroSeconds iSeekPos;
		
		TInt					iKeyRepeatCount;

		TBool					iSeeking;
		TBool					iBufferingCompleted;
		
		TBool					iMuted;
		
		HBufC* 						iFilename;
		TTimeIntervalMicroSeconds	iStartPoint;
		TTimeIntervalMicroSeconds	iEndPoint;
		
		TBool						iNewFile;

		/** Callback utility for setting the animation frame interval*/
		CAsyncCallBack* 			iCallBack;
    };
#endif

// End of File
