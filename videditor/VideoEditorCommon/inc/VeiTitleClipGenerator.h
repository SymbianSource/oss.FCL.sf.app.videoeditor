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



#ifndef __VEITITLECLIPGENERATOR_H__
#define __VEITITLECLIPGENERATOR_H__

#include <e32base.h>

#include <VedVideoClipGenerator.h>

/*
 * UID of this generator. The Uids are only used to identify generators
 * in UI, the Engine does not use Uids for any purpose.
 */
#define KUidTitleClipGenerator TUid::Uid(0x00000002)


// Forward declarations

class CVeiTitleClipGenerator;
//class CVeiTitleClipMainTitleFrameOperation;
//class CVeiTitleClipScrollerFrameOperation;
class CVeiTitleClipImageDecodeOperation;
class CFbsBitmap;
class CBitmapScaler;
class CImageDecoder;
class CFont;

// Enumerations

/**
 * Title clip style.
 */
enum TVeiTitleClipVerticalAlignment 
	{
	EVeiTitleClipVerticalAlignmentTop,
	EVeiTitleClipVerticalAlignmentBottom,
	EVeiTitleClipVerticalAlignmentCenter
	};

/**
 * Title clip justification.
 */
enum TVeiTitleClipHorizontalAlignment
	{
	EVeiTitleClipHorizontalAlignmentLeft,
	EVeiTitleClipHorizontalAlignmentRight,
	EVeiTitleClipHorizontalAlignmentCenter
	};

/**
 * Title clip transition.
 */
enum TVeiTitleClipTransition 
	{
	EVeiTitleClipTransitionNone = 0,
	EVeiTitleClipTransitionScrollLeftToRight,
	EVeiTitleClipTransitionScrollRightToLeft,
	EVeiTitleClipTransitionScrollTopToBottom,
	EVeiTitleClipTransitionScrollBottomToTop,
	EVeiTitleClipTransitionFade
	};



/**
 * Observer for title clip generator.
 */
class MVeiTitleClipGeneratorObserver
	{
public:
	/**
	 * Notifies that the title clip generator has finished loading and
	 * preparing the background image.
	 *
	 * @param aGenerator  generator that caused the event
	 * @param aError      error code
	 */
	virtual void NotifyTitleClipBackgroundImageLoadComplete(CVeiTitleClipGenerator& aGenerator, TInt aError) = 0;
	};

/**
 * Title clip generator.
 */
class CVeiTitleClipGenerator : public CVedVideoClipGenerator
	{
public:

	/* Constructors / destructors. */

	IMPORT_C static CVeiTitleClipGenerator* NewL(const TSize& aMaxResolution, 
		TVeiTitleClipTransition aTransition, 
		TVeiTitleClipHorizontalAlignment aHorizontalAlignment,
		TVeiTitleClipVerticalAlignment aVerticalAlignment);

	IMPORT_C static CVeiTitleClipGenerator* NewLC(const TSize& aMaxResolution,
		TVeiTitleClipTransition aTransition,
		TVeiTitleClipHorizontalAlignment aHorizontalAlignment,
		TVeiTitleClipVerticalAlignment aVerticalAlignment);

	IMPORT_C virtual ~CVeiTitleClipGenerator();

	/* Property methods. */

	IMPORT_C virtual TPtrC DescriptiveName() const;

	IMPORT_C virtual TUid Uid() const;
	
    IMPORT_C virtual TTimeIntervalMicroSeconds Duration() const;


	/* Video frame property methods. */

    IMPORT_C virtual TInt VideoFrameCount() const;

	IMPORT_C virtual TTimeIntervalMicroSeconds VideoFrameStartTime(TInt aIndex) const;

	IMPORT_C virtual TTimeIntervalMicroSeconds VideoFrameEndTime(TInt aIndex) const;

	IMPORT_C virtual TTimeIntervalMicroSeconds VideoFrameDuration(TInt aIndex) const;

    IMPORT_C virtual TInt GetVideoFrameIndex(TTimeIntervalMicroSeconds aTime) const;

	IMPORT_C virtual TInt VideoFirstFrameComplexityFactor() const;

	IMPORT_C virtual TInt VideoFrameDifferenceFactor(TInt aIndex) const;


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
	 * Sets the text for the generator.
	 *
	 * @param aText  text for the generator
	 */
	IMPORT_C void SetTextL(const TDesC& aText);

	/**
	 * Returns the text for this generator.
	 *
	 * @return  text for the generator
	 */
	IMPORT_C TPtrC Text() const;

	/**
	 * Returns the transition of this title screen clip.
	 *
	 * @return  transition
	 */
	IMPORT_C TVeiTitleClipTransition Transition() const;

	/**
	 * Returns the horizontal alignment.
	 * 
	 * @return  horizontal alignment
	 */
	IMPORT_C TVeiTitleClipHorizontalAlignment HorizontalAlignment() const;

	/**
	 * Returns the vertical alignment.
	 * 
	 * @return  vertical alignment
	 */
	IMPORT_C TVeiTitleClipVerticalAlignment VerticalAlignment() const;

	/**
	 * Sets the transiton and alignments. They are all tied together
	 * to allow them to be set at once to avoid generating several 
	 * thumbnails when setting many options at the same time.
	 *
	 * @param aTransition  transition for the generator.
	 * @param aHorizontalAlignment  horizontal alignment
	 * @param aVerticalAlignment  vertical alignment
	 */
	IMPORT_C void SetTransitionAndAlignmentsL(TVeiTitleClipTransition aTransition,
		TVeiTitleClipHorizontalAlignment aHorizontalAlignment,
		TVeiTitleClipVerticalAlignment aVerticalAlignment);

	/**
	 * Returns the background color.
	 *
	 * @return  background color.
	 */
	IMPORT_C TRgb BackgroundColor() const;

	/**
	 * Sets the background color. This is used only if no background image
	 * is set.
	 *
	 * @param aBacgroundColor  background color
	 */ 
	IMPORT_C void SetBackgroundColorL(const TRgb& aBackgroundColor);

	/**
	 * Returns the text color.
	 *
	 * @return  text color.
	 */
	IMPORT_C TRgb TextColor() const;

	/**
	 * Sets the text color. 
	 *
	 * @param aBacgroundColor  text color
	 */ 
	IMPORT_C void SetTextColorL(const TRgb& aTextColor);

	/**
	 * Sets the background image. Transfers the ownership of the bitmap
	 * to the generator.
	 *
	 * @param aBackgroundImage  background image
	 */
	IMPORT_C void SetBackgroundImageL(const CFbsBitmap* aBackgroundImage);

	/**
	 * Sets the background image from the specified file. 
	 *
	 * @param aFilename  filename for the image to load
	 * @param aObserver  observer to notify when loading is complete
	 */
	IMPORT_C void SetBackgroundImageL(const TDesC& aFilename, MVeiTitleClipGeneratorObserver& aObserver);

	/**
	 * Returns the background image.
	 *
	 * @return  background image
	 */
	IMPORT_C CFbsBitmap* BackgroundImage() const;

	/**
	 * Sets the duration.
	 *
	 * @param aDuration  duration
	 */
	IMPORT_C void SetDuration(const TTimeIntervalMicroSeconds& aDuration);

	/**
	 * Sets the descriptive name.
	 *
	 * @param aDescriptiveName  descriptive name
	 */
	IMPORT_C void SetDescriptiveNameL(const TDesC& aDescriptiveName);

private: 
	// from MVedVideoClipGeneratorFrameObserver	
	void NotifyVideoClipGeneratorFrameCompleted(CVedVideoClipGenerator& aGenerator, TInt aError, CFbsBitmap* aBitmap);

private: // constructors
	
	/**
	 * First-phase constructor. 
	 * 
	 * @param aMaxResolution		maximum resolution
	 * @param aTransition			transition
	 * @param aHorizontalAlignment	horizontal alignment
	 * @param aVerticalAlignment	vertical alignment
	 */
	CVeiTitleClipGenerator(const TSize& aMaxResolution, 
						   TVeiTitleClipTransition aTransition, 
						   TVeiTitleClipHorizontalAlignment aHorizontalAlignment,
						   TVeiTitleClipVerticalAlignment aVerticalAlignment);

	/**
	 * Second-phase constructor.
	 */
	void ConstructL();

private: // new methods

	/**
	 * Wrap the input text into lines.
	 */
	void WrapTextToArrayL(const TDesC& aText);

	/**
	 * Updates the first frame complexity factor value.
	 */
	void UpdateFirstFrameComplexityFactorL();

	/**
	 * Calculates the fading transition start and end indices.
	 *
	 * @param  aInEndFrame     for returning the end frame of in-transition
	 * @param  aOutStartFrame  for returning the start frame of out-transition
	 * @param  aNumberOfFrames for returning number of frames
	 */
	void CalculateTransitionFrameIndices(TInt& aInEndFrame, TInt& aOutStartFrame) const;

	/**
	 * Gets the text font suitable for the current frame resolution.
	 */
	void GetTextFont(TInt aFontDivisor = 0);

	/**
	 * Draws wrapped texts on the specified bitmap.
	 *
	 * @param aBitmap       bitmap to draw on
	 * @param aTextPoint    text starting point
	 * @param aTextColor    text color
	 * @param aBgColor      background color
	 * @param aShadowColor  shadow color
	 */
	void DrawWrappedTextL(CFbsBitmap& aBitmap, const TPoint& aTextPoint, const TRgb& aTextColor,
		const TRgb& aBgColor, const TRgb& aShadowColor, TBool aDrawBackground);

	/**
	 * Finishes the GetFrameL() call.
	 */
	CFbsBitmap* FinishGetFrameL(TInt aError = KErrNone);

	/**
	 * Synchronous method for getting the first frame for complexity
	 * calculation.
	 *
	 * @return  first frame
	 */
	CFbsBitmap* GetFirstFrameL();

	/**
	 * Draws the main title frame on a specified bitmap.
	 *
	 * @param aBitmap  bitmap to draw on
	 * @param aIndex   index of the frame to draw
	 */
	void DrawMainTitleFrameL(CFbsBitmap& aBitmap, TInt aIndex);

	/**
	 * Draws the scroll title frame on a specified bitmap.
	 *
	 * @param aBitmap  bitmap to draw on
	 * @param aIndex   index of the frame to draw
	 */
	void DrawScrollTitleFrameL(CFbsBitmap& aBitmap, TInt aIndex);

	/**
	 * Get the maximum frame rate, which is either the movie's 
	 * frame rate, of the hard-coded maximum value.
	 *
	 * @return  max frames per second
	 */
	TInt MaximumFramerate() const;

private:
    // Member variables

	/** First frame complexity factor. */
	TInt iFirstFrameComplexityFactor;

	/** Text. */
	HBufC* iText;

	/** Font for the text */
	const CFont* iTextFont;

	/** Resolution used to calculate font size. */
	TSize iGetFontResolution;

	/** Desciptive name. */
	HBufC* iDescriptiveName;

	/** Wrapped text pointer array. */
	CArrayFix<TPtrC>* iWrappedArray;

	/** Height of the wrapped text box. */
	TInt iWrappedTextBoxHeight;

	/** Resolution. */
	TSize iMaxResolution;

	/** Duration. */
	TTimeIntervalMicroSeconds iDuration;

	/** Transition. */
	TVeiTitleClipTransition iTransition;

	/** Horizontal alignment. */
	TVeiTitleClipHorizontalAlignment iHorizontalAlignment;

	/** Vertical alignment. */
	TVeiTitleClipVerticalAlignment iVerticalAlignment;

	/** Background image decode operation. */
	CVeiTitleClipImageDecodeOperation* iDecodeOperation;
	
	/** Background color. */
	TRgb iBackgroundColor;
	
	/** Text color. */
	TRgb iTextColor;

	/** Flag for notifying setting change. */
	TBool iSettingsChanged;

	/** Background image. */
	CFbsBitmap* iBackgroundImage;

	/** Scaled background image. */
	CFbsBitmap* iScaledBackgroundImage;

	/** Flag indicating that we should use the scaled bitmap. */
	TBool iUseScaledImage;


	/* 
	 * Following member variables are temporary storage for the two-phase 
	 * GetFrameL. 
	 */

	/** Observer. */
	MVedVideoClipGeneratorFrameObserver* iGetFrameObserver;
	/**	Index. */
    TInt iGetFrameIndex;
	/** Resolution. */
	TSize iGetFrameResolution;
	/** DisplayMode. */
	TDisplayMode iGetFrameDisplayMode;
	/** Enhance flag. */
	TBool iGetFrameEnhance;
	/** Priority. */
	TInt iGetFramePriority;

	/* Operation classes need to be our friends. */
	friend class CVeiTitleClipImageDecodeOperation;
	};

/**
 * Image decode operation. Helper class for decoding the image and 
 * performing first-stage resizing.
 */
class CVeiTitleClipImageDecodeOperation : public CActive
	{
	public: 
		/**
		 * Factory constructor method. Constructs a decode operation.
		 *
		 * @param aGenerator  generator that owns this operation 
		 * @param aObserver   observer that will be notified when the
		 *                    operation is complete
		 * @param aFilename   filename of the image to be decoded
		 * @param aPriority   priority of the active object
		 *
		 * @return  constructed instance of decode operation
		 */
		static CVeiTitleClipImageDecodeOperation* NewL(CVeiTitleClipGenerator& aGenerator,
													   MVeiTitleClipGeneratorObserver& aObserver,
													   const TDesC& aFilename,
													   TInt aPriority = CActive::EPriorityStandard);


		/**
		 * Factory constructor method. Constructs a decode operation.
		 *
		 * @param aGenerator  generator that owns this operation 
		 * @param aSourceBitmap source bitmap
		 * @param aPriority   priority of the active object
		 *
		 * @return  constructed instance of decode operation
		 */
		static CVeiTitleClipImageDecodeOperation* NewL(CVeiTitleClipGenerator& aGenerator,
													   CFbsBitmap* aSourceBitmap,
													   TInt aPriority = CActive::EPriorityStandard);

		/**
		 * Destructor.
		 */
		virtual ~CVeiTitleClipImageDecodeOperation();

		virtual void RunL();
		virtual void DoCancel();
		virtual TInt RunError(TInt aError);

		/**
		 * Starts the loading operation. 
		 * 
		 * @param aMaxResolution  maximum resolution that will be used
		 */
		void StartLoadOperationL(const TSize& aMaxResolution);

		/**
		 * Starts the scaling operation. 
		 * 
		 * @param aResolution  resolution for the image
		 */
		void StartScalingOperationL(const TSize& aResolution);

	private: // methods
		/**
		 * EPOC first-phase constructor.
		 *
		 * @param aGenerator  generator that owns this decode operation
		 * @param aObserver   observer to notify when the operation is
		 *                    complete
		 * @param aPriority   priority for the active object
		 */
		CVeiTitleClipImageDecodeOperation(CVeiTitleClipGenerator& aGenerator,
										  MVeiTitleClipGeneratorObserver& aObserver,
										  TInt aPriority = CActive::EPriorityStandard);

		/**
		 * EPOC first-phase constructor.
		 *
		 * @param aGenerator    generator that owns this decode operation
		 * @param aSourceBitmap source bitmap
		 * @param aPriority     priority for the active object
		 */
		CVeiTitleClipImageDecodeOperation(CVeiTitleClipGenerator& aGenerator, 
										  CFbsBitmap* aSourceBitmap,
										  TInt aPriority = CActive::EPriorityStandard);
		/**
		 * EPOC second-phase constructor.
		 * 
		 * @param aFilename  filename to decode.
		 */
		void ConstructL(const TDesC& aFilename);

	private: // members

		/**
		 * Enumeration for the decode phase.
		 */
		enum TDecodePhase 
			{
			EPhaseNotStarted = 0,
			EPhaseLoading,
			EPhaseScaling,
			EPhaseComplete
			};

		/** Decode phase. */
		TDecodePhase iDecodePhase;

		/** Generator that owns this decoder operation. */
		CVeiTitleClipGenerator& iGenerator;

		/** Observer.*/
		MVeiTitleClipGeneratorObserver* iObserver;

		/** Decoder. */
		CImageDecoder* iDecoder;

		/** Bitmap. */
		CFbsBitmap* iBitmap;

		/** Scaler. */
		CBitmapScaler* iScaler;

		/** Flag indicating whether to notify observer. */
		TBool iNotifyObserver;
	};




#endif // __VEITITLECLIPGENERATOR_H__

