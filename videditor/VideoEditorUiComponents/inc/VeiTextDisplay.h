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



#ifndef VEITEXTDISPLAY_H
#define VEITEXTDISPLAY_H

#include <coecntrl.h>
#include <ConeResLoader.h>


/**
 * CVeiTextDisplay control class.
 */
class CVeiTextDisplay : public CCoeControl
    {
    public:
        /**
         * Destructor.
         */
        IMPORT_C virtual ~CVeiTextDisplay();

		/** 
		 * Static factory method.
		 * 
		 * @return  the created CVeiTextDisplay object
		 */
		IMPORT_C static CVeiTextDisplay* NewL( const TRect& aRect, const CCoeControl* aParent );

		/** 
		 * Static factory method. Leaves the created object in the cleanup
		 * stack.
		 *
		 * @return  the created CVeiCutAudioBar object
		 */
		IMPORT_C static CVeiTextDisplay* NewLC( const TRect& aRect, const CCoeControl* aParent );


	public:

		enum TVeiLayout
			{
			EOnlyName = 0x77,
			ENameAndDuration,
			EEverything,
			ECutInCutOut,
			EArrowsHorizontal,
			EArrowsVertical,
			ERecording,
			ERecordingPaused,
            EOnlyDuration
			};
			
    	/**
		 * CVeiTextDisplay components.
		 */
		 enum TTextDisplayComponent
			{
			EStartTimeText= 1,
			EEndTimeText,
			EStartTimeIcon,
			EEndTimeIcon			
			};			

		IMPORT_C void SetLandscapeScreenOrientation( TBool aLandscapeScreenOrientation );

		IMPORT_C void SetCutIn( const TTimeIntervalMicroSeconds& aCutInTime );
		
		IMPORT_C void SetCutOut( const TTimeIntervalMicroSeconds& aCutOutTime );

		IMPORT_C void SetTime( const TTime& aClipTime );

		IMPORT_C void SetLocation( const TDesC& aClipLocation );

		IMPORT_C void SetLayout( TVeiLayout aLayout );

		IMPORT_C void SetName( const TDesC& aName );

		IMPORT_C void SetDuration( const TTimeIntervalMicroSeconds& aDuration );

		/** 
		 * Control Up arrow visibility.
		 * 
		 * @param aVisible True/False
		 */
		IMPORT_C void SetUpperArrowVisibility(TBool aVisible);

		/** 
		 * Control Lower arrow visibility.
		 * 
		 * @param aVisible True/False
		 */
		IMPORT_C void SetLowerArrowVisibility(TBool aVisible);

		/** 
		 * Control Right arrow visibility.
		 * 
		 * @param aVisible True/False
		 */
		IMPORT_C void SetRightArrowVisibility(TBool aVisible);

		/** 
		 * Control Left arrow visibility.
		 * 
		 * @param aVisible True/False
		 */
		IMPORT_C void SetLeftArrowVisibility(TBool aVisible);
		
		/** 
		 * Set slow motion on status.
		 * 
		 * @param aOn True/False
		 */
		IMPORT_C void SetSlowMotionOn(TBool aOn);

		/** 
		 * slow motion on status.
		 * 
		 * @return aOn True/False
		 */
		IMPORT_C TBool SlowMotionOn() const;
		
		/** 
		 * Set value of slow motion effect.
		 * 
		 * @param aPreset
		 */
		IMPORT_C void SetSlowMotionPreset(TInt aPreset);

		/** 
		 * Slow motion effect value.
		 * 
		 * @return value 
		 */
		IMPORT_C TInt SlowMotionPreset() const;

		IMPORT_C void SetArrowSize(const TSize& aArrowSize);

		void ParseTimeToMinSec( TDes& aLayoutTime, const TTimeIntervalMicroSeconds& aDuration ) const;
		
		/** 
		 * Sets the rect of a component
		 * 
		 * @param aComponentIndex specifies the component
		 * @param aRect the rect that the component is set
		 *
		 * @return  the end mark position
		 */		
		IMPORT_C void SetComponentRect(TTextDisplayComponent aComponentIndex, TRect aRect);

    private:
        /**
         * Default constructor.
		 *
         */
        void ConstructL( const TRect& aRect, const CCoeControl* aParent );

        /**
         * C++ default constructor.
		 *
         */
		CVeiTextDisplay();

       /**
        * From CCoeControl,Draw.
		*
		* @param aRect  rectangle to draw
        */
        void Draw(const TRect& aRect) const;

		static TInt UpdateBlinker( TAny* aThis );
		void DoUpdateBlinker();
		void SizeChanged();

    private:	// data
		HBufC*		iClipName;
		TTimeIntervalMicroSeconds iDuration;
		TTime		iClipTime;		
		HBufC*		iClipLocation;
		
		TTimeIntervalMicroSeconds	iCutInTime;
		TTimeIntervalMicroSeconds	iCutOutTime;

		TVeiLayout	iLayout;

		CFbsBitmap* iUpperArrow;
		CFbsBitmap* iLowerArrow;
		CFbsBitmap* iRightArrow;
		CFbsBitmap* iLeftArrow;
		CFbsBitmap* iUpperArrowMask;
		CFbsBitmap* iLowerArrowMask;
		CFbsBitmap* iRightArrowMask;
		CFbsBitmap* iLeftArrowMask;
		CFbsBitmap* iStartMarkIcon;
		CFbsBitmap* iStartMarkIconMask;
		CFbsBitmap* iEndMarkIcon;
		CFbsBitmap* iEndMarkIconMask;

		TBool		iUpperArrowVisible;
		TBool		iLowerArrowVisible;
		TBool		iRightArrowVisible;
		TBool		iLeftArrowVisible;

		TBool		iLandscapeScreenOrientation;
		TBool		iSlowMotionOn;
		TInt		iPresetValue;	
		
		CPeriodic*	iBlinkTimer;
		TBool		iBlinkFlag;

		TSize		iDynamicArrowSize;
		
		TPoint 		iUpperArrowPoint;
		TPoint 		iLowerArrowPoint;

		RConeResourceLoader 	iResLoader;

		TRect	iStartTimeIconRect;
    	TRect	iEndTimeIconRect;
       	TRect	iStartTimeTextRect;
    	TRect	iEndTimeTextRect;

    };
#endif

// End of File
