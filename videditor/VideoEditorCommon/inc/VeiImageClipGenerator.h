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



#ifndef __VEIIMAGECLIPGENERATOR_H__
#define __VEIIMAGECLIPGENERATOR_H__

#include <e32base.h>

#include <VedVideoClipGenerator.h>

/*
 * UID of this generator. The Uids are only used to identify generators
 * in UI, the Engine does not use Uids for any purpose.
 */
#define KUidImageClipGenerator TUid::Uid(0x00000001)


// Forward declarations
class CVeiImageClipFrameOperation;
class CVeiImageClipDecodeOperation;
class CFbsBitmap;
class CImageDecoder;
class CVeiImageClipGenerator;
class CBitmapScaler;


/**
 * Image clip generator observer.
 */
class MVeiImageClipGeneratorObserver
	{
public:
	/**
	 * Callback method to notify that the image clip generator initialization
	 * is complete.
	 *
	 * @param aGenerator  generator that caused the event
	 * @param aError	  error code
	 */
	virtual void NotifyImageClipGeneratorInitializationComplete(
		CVeiImageClipGenerator& aGenerator, TInt aError) = 0;
	};


/**
 * Image clip generator.
 */
class CVeiImageClipGenerator : public CVedVideoClipGenerator
	{
public:

	/* Constructors / destructors. */

	/**
	 * Factory method. Creates a new image clip generator.
	 * 
	 * @param aFilename		  filename for the image
	 * @param aMaxResolution  maximum resolution that will be used
	 * @param aDuration		  duration of the image frame
	 * @param aBackgroundColor  background color
	 * @param aMaxDisplayMode	display mode in which the bitmap is stored
	 * @param aFs			  RFs session to use for loading the image
	 * @param aObserver		  observer that will be notified when the
	 *						  initialization is complete
	 *
	 * @return  constructed image clip generator
	 */
	IMPORT_C static CVeiImageClipGenerator* NewL(const TDesC& aFilename,
												 const TSize& aMaxResolution, 
												 const TTimeIntervalMicroSeconds& aDuration, 
												 const TRgb& aBackgroundColor,
												 TDisplayMode aMaxDisplayMode,
												 RFs& aFs,
												 MVeiImageClipGeneratorObserver& aObserver);
		
	/**
	 * Factory method. Creates a new image clip generator and leaves it
	 * in the cleanup stack.
	 * 
	 * @param aFilename		    filename for the image
	 * @param aMaxResolution    maximum resolution that will be used
	 * @param aDuration		    duration of the image frame
	 * @param aBackgroundColor  background color
	 * @param aMaxDisplayMode	display mode in which the bitmap is stored
	 * @param aFs			    RFs session to use for loading the image
	 * @param aObserver		    observer that will be notified when the
	 *						    initialization is complete
	 *
	 * @return  constructed image clip generator
	 */
	IMPORT_C static CVeiImageClipGenerator* NewLC(const TDesC& aFilename,
												  const TSize& aMaxResolution,
												  const TTimeIntervalMicroSeconds& aDuration, 
												  const TRgb& aBackgroundColor,
												  TDisplayMode aMaxDisplayMode,
 												  RFs& aFs,
												  MVeiImageClipGeneratorObserver& aObserver);

	/**
	 * Destructor.
	 */
	IMPORT_C virtual ~CVeiImageClipGenerator();


	/* Property methods. */

	IMPORT_C virtual TPtrC DescriptiveName() const;

	IMPORT_C virtual TUid Uid() const;

    IMPORT_C virtual TTimeIntervalMicroSeconds Duration() const;


	/* Video frame property methods. */

    IMPORT_C virtual TInt VideoFrameCount() const ;

	IMPORT_C virtual TTimeIntervalMicroSeconds VideoFrameStartTime(TInt aIndex) const;

	IMPORT_C virtual TTimeIntervalMicroSeconds VideoFrameEndTime(TInt aIndex) const;

	IMPORT_C virtual TTimeIntervalMicroSeconds VideoFrameDuration(TInt aIndex) const;

	IMPORT_C virtual TBool VideoFrameIsIntra(TInt aIndex) const;

	IMPORT_C virtual TInt VideoFirstFrameComplexityFactor() const;

	IMPORT_C virtual TInt VideoFrameDifferenceFactor(TInt aIndex) const;

    IMPORT_C virtual TInt GetVideoFrameIndex(TTimeIntervalMicroSeconds aTime) const;


	/* Frame methods. */

	IMPORT_C virtual void GetFrameL(MVedVideoClipGeneratorFrameObserver& aObserver,
                           TInt aIndex,
		                   TSize* const aResolution,
						   TDisplayMode aDisplayMode,
						   TBool aEnhance,
						   TInt aPriority);
    
	IMPORT_C virtual void CancelFrame();


	/* New methods. */

	/**
	 * Sets the duration.
	 *
	 * @param aDuration  duration
	 */
	IMPORT_C void SetDuration(const TTimeIntervalMicroSeconds& aDuration);

	/**
	 * Sets the background color.
	 *
	 * @param aBackgroundColor
	 */
	IMPORT_C void SetBackgroundColor(const TRgb& aBackgroundColor);

	/**
	 * Gets the background color.
	 * 
	 * @return  background color
	 */
	IMPORT_C const TRgb& BackgroundColor() const;

	/**
	 * Returns the image file name.
	 *
	 * @return  filename
	 */
	IMPORT_C TPtrC ImageFilename() const;

private: // constructors
	
	/**
	 * First-phase constructor.
	 * 
	 * @param aDuration        duration
	 * @param aBackgroundColor background color 
	 * @param aMaxResolution   maximum resolution
	 */
	CVeiImageClipGenerator(const TTimeIntervalMicroSeconds& aDuration, 
						   const TRgb& aBackgroundColor,
						   const TSize& aMaxResolution);

	/**
	 * Second-phase constructor.
	 * 
	 * @param aFilename       filename of the image
	 * @param aObserver       observer
	 * @param aMaxDisplyMode  display mode
	 * @param aFs             RFs session
	 */
	void ConstructL(const TDesC& aFilename, 
					MVeiImageClipGeneratorObserver& aObserver,
					TDisplayMode aMaxDisplayMode, RFs& aFs);

	
private:
	/**
	 * Updates the first frame complexity factor.
	 */
	void UpdateFirstFrameComplexityFactorL();

private:
    // Member variables

	/** Flag for readiness indication. */
	TBool iReady;

	/** First frame complexity factor. */
	TInt iFirstFrameComplexityFactor;

	/** Resolution. */
	TSize iMaxResolution;

	/** Duration. */
	TTimeIntervalMicroSeconds iDuration;

	/** Background color. */
	TRgb iBackgroundColor;

	/** Image decode operation. */
	CVeiImageClipDecodeOperation* iDecodeOperation;

	/** Frame generating operation. */
	CVeiImageClipFrameOperation* iFrameOperation;

	/** Descriptive name. */
	HBufC* iDescriptiveName;

	/** Image filename */
	HBufC* iFilename;
	
	/** Bitmap. */
	CFbsBitmap* iBitmap;

	/** Mask. */
	CFbsBitmap* iMask;

	/** Is the generator initializing? */
	TBool iInitializing;

	/** Frame count. */
	TInt iFrameCount;

	friend class CVeiImageClipDecodeOperation;
	};


/**
 * Image decode operation. Helper class for decoding the image and 
 * performing first-stage resizing.
 */
class CVeiImageClipDecodeOperation : public CActive
	{
	public: 
		/**
		 * Factory constructor. 
		 * 
		 * @param aGenerator  generator that owns this operation
		 * @param aFilename	  filename for the image
		 * @param aObserver	  observer that will be notified when the
		 *					  operation completes
		 * @param aFs		  RFs session to use for loading the image
		 * @param aPriority   priority of the active object
		 *
		 * @return  constructed image clip decode operation
		 */
		static CVeiImageClipDecodeOperation* NewL(CVeiImageClipGenerator& aGenerator,
												  const TDesC& aFilename, 
												  MVeiImageClipGeneratorObserver& aObserver,
												  RFs& aFs,
												  TInt aPriority = CActive::EPriorityStandard);
		/**
		 * Destructor.
		 */
		virtual ~CVeiImageClipDecodeOperation();

		virtual void RunL();
		virtual void DoCancel();
		virtual TInt RunError(TInt aError);

		/**
		 * Starts the operation.
		 * 
		 * @param aMaxResolution  maximum resolution that will be used
		 * @param aDisplayMode	  display mode
		 */
		void StartOperationL(const TSize& aMaxResolution, TDisplayMode aDisplayMode);

	private: // methods

		/**
		 * First-phase constructor.
		 *
		 * @param aGenerator  generator that owns this operation
		 * @param aObserver	  observer that will be notified when the
		 *					  operation completes
		 * @param aPriority   priority of the active object
		 */
		CVeiImageClipDecodeOperation(CVeiImageClipGenerator& aGenerator, 
									 MVeiImageClipGeneratorObserver& aObserver,
									 TInt aPriority);

		/**
		 * Second-phase constructor.
		 *
 		 * @param aFilename	  filename for the image
		 * @param aFs		  RFs session to use for loading the image
		 */

		void ConstructL(const TDesC& aFilename, RFs& aFs);

	private: // members

		/** Generator that owns this operation*/
		CVeiImageClipGenerator& iGenerator;

		/** Observer to be notified when the operation completes. */
		MVeiImageClipGeneratorObserver& iObserver;

		/** Decoder. */
		CImageDecoder* iDecoder;

		/** Bitmap. */
		CFbsBitmap* iBitmap;

		/** Mask. */
		CFbsBitmap* iMask;
	};


/**
 * Image frame operation.
 */
class CVeiImageClipFrameOperation : public CActive
	{
	public: 
		/**
		 * Factory constructor. 
		 * 
		 * @param aGenerator  generator that owns this operation
		 *
		 * @return  constructed image clip frame operation
		 */
		static CVeiImageClipFrameOperation* NewL(CVeiImageClipGenerator& aGenerator);

		/**
		 * Destructor. 
		 */
		virtual ~CVeiImageClipFrameOperation();

		/**
		 * Starts the operation.
		 *
		 * @param aObserver      observer to notify when the frame is complete
		 * @param aIndex         index of the frame to generate
		 * @param aEnhance       <code>ETrue</code> to produce better results
		 *                       but slower, <code>EFalse</code> to be as fast
		 *                       as possible.
		 * @param aSourceBitmap  source bitmap
		 * @param aDestBitmap    destination bitmap
		 * @param aSourceMask    source mask
		 * @param aPriority      priority for the active object
		 *
		 */
		void StartOperationL(MVedVideoClipGeneratorFrameObserver* aObserver, 
			TInt aIndex, TBool aEnhance, CFbsBitmap* aSourceBitmap, 
			CFbsBitmap* aDestBitmap, CFbsBitmap* aSourceMask, TInt aPriority);

		virtual void RunL();
		virtual void DoCancel();
		virtual TInt RunError(TInt aError);

	private: // methods
		/**
		 * First-phase constructor.
		 * 
		 * @param aGenerator  generator that owns this operation
		 */
		CVeiImageClipFrameOperation(CVeiImageClipGenerator& aGenerator);

		/**
		 * Second-phase constructor.
		 */
		void ConstructL();

	private: // members
		/** Generator. */
		CVeiImageClipGenerator& iGenerator;

		/** Index of the frame being generated. */
		TInt iIndex;

		/** Whether to be fast or do quality work. */
		TBool iEnhance;

		/** Source bitmap. */
		CFbsBitmap* iSourceBitmap;

		/** Destination bitmap. */
		CFbsBitmap* iDestBitmap;

		/** Scaled bitmap. */
		CFbsBitmap* iScaledBitmap;

		/** Scaled mask. */
		CFbsBitmap* iScaledMask;

		/** Source mask. */
		CFbsBitmap* iSourceMask;

		/** Scaler. */
		CBitmapScaler* iScaler;

		/** Flag for no scaling. */
		TBool iNoScaling;

		/** Observer. */
		MVedVideoClipGeneratorFrameObserver* iObserver;
	};



#endif // __VEIIMAGECLIPGENERATOR_H__

