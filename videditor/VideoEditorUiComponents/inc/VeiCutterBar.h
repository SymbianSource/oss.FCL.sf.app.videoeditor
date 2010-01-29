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
* Declares CVeiCutterBar control for the Video Editor.
*
*/



#ifndef VEICUTTERBAR_H
#define VEICUTTERBAR_H

#include <coecntrl.h>

class CPeriodic;

/**
 * CVeiCutterBar control class.
 */
class CVeiCutterBar : public CCoeControl, public MCoeControlObserver
    {
    public:
    		/**
		 * CVeiCutterBar components.
		 */
		 enum TCutterBarComponent
			{
			EScissorsIcon = 1,
			EProgressBar,
			ESliderLeftEndIcon,
			ESliderMiddleIcon,
			ESliderRightEndIcon,
			ESliderSelectedLeftEndIcon,
			ESliderSelectedMiddleIcon,
			ESliderSelectedRightEndIcon,
			EPlayheadIcon,
			ECutAreaBorderIcon,
			EStartMarkIcon,
			EEndMarkIcon,
			EPlayheadTouch,
			EStartMarkTouch,
			EEndMarkTouch
			};
        
        // CVeiCutterBar icons that can be in pressed state
        enum TCutterBarPressedIcon
			{
			ENoPressedIcon = 0,
			EPressedPlayheadTouch,
			EPressedStartMarkTouch,
			EPressedEndMarkTouch
			};
    public:
        /**
         * Destructor.
         */
        IMPORT_C virtual ~CVeiCutterBar();

		/** 
		 * Static factory method.
		 * 
		 * @param aCont  pointer to the container
		 *
		 * @return  the created CVeiCutterBar object
		 */
		IMPORT_C static CVeiCutterBar* NewL( const CCoeControl* aParent, TBool aDrawBorder = EFalse );

		/** 
		 * Static factory method. Leaves the created object in the cleanup
		 * stack.
		 * 
		 * @param aCont  pointer to the container
		 *
		 * @return  the created CVeiCutterBar object
		 */
		IMPORT_C static CVeiCutterBar* NewLC( const CCoeControl* aParent, TBool aDrawBorder = EFalse  );

    public:

		IMPORT_C virtual void SetPlayHeadVisible( TBool aVisible );
		/**
		 * Sets the mark in point.
		 *
		 * @param aIn  new In-point
		 */
		IMPORT_C virtual void SetInPoint( const TTimeIntervalMicroSeconds& aIn );

		/**
		 * Sets the mark out point.
		 *
		 * @param aOut  new Out-point
		 */
		IMPORT_C virtual void SetOutPoint( const TTimeIntervalMicroSeconds& aOut );

		/**
		 * Sets the "finished" status, i.e., if the clip is finished, the
		 * leftover areas outside in/out points are grayed out.
		 *
		 * @param aStatus  <code>ETrue</code> for "is finished";
		 *                 <code>EFalse</code> for "not finished"
		 */
		IMPORT_C virtual void SetFinishedStatus( TBool aStatus );

		IMPORT_C virtual void SetTotalDuration( const TTimeIntervalMicroSeconds& aDuration );

		IMPORT_C virtual void SetCurrentPoint( TInt aLocation );

		IMPORT_C virtual void Dim( TBool aDimmed );

		/** 
		 * Getter for iCutBarRect
		 * CVeiCutterBar's rect covers also the scissor icon area but iCutBarRect 
		 * is the visible area of the progress bar
		 * 
		 * @param -
		 *
		 * @return  the progress bar rect
		 */
		IMPORT_C TRect ProgressBarRect(); 
		
		/** 
		 * Returns the playhead rectangle.
		 * If the playheadhasn't been set, returns an empty rect
		 * 
		 * @param -
		 *
		 * @return  playhead rect or empty rect
		 */		
		IMPORT_C TRect PlayHeadRect(); 

		/** 
		 * Returns the start mark rectangle.
		 * If the start mark hasn't been set, returns an empty rect
		 * 
		 * @param -
		 *
		 * @return  start mark rect or empty rect
		 */		
		IMPORT_C TRect StartMarkRect(); 

		/** 
		 * Returns the end mark rectangle.
		 * If the end mark hasn't been set, returns an empty rect
		 * 
		 * @param -
		 *
		 * @return  end mark rect or empty rect
		 */		
		IMPORT_C TRect EndMarkRect(); 

		/** 
		 * Returns the start mark position in progress bar
		 * 
		 * @param -
		 *
		 * @return  the start mark position
		 */		
		IMPORT_C TUint StartMarkPoint(); 
		
		/** 
		 * Returns the end mark position in progress bar
		 * 
		 * @param -
		 *
		 * @return  the end mark position
		 */		
		IMPORT_C TUint EndMarkPoint(); 

		/** 
		 * Sets the rect of a component
		 * 
		 * @param aComponentIndex specifies the component
		 * @param aRect the rect that the component is set
		 *
		 * @return  the end mark position
		 */		
		IMPORT_C void SetComponentRect(TCutterBarComponent aComponentIndex, TRect aRect);
		
		/** 
		 * Sets one of the cutterbar components to pressed state. This function
		 * can also be used to set ENoPressedIcon as pressed component (= none
		 * of the components is pressed) 
		 * 
		 * @param aComponentIndex specifies the component that should be 
		 *          set to pressed state
		 *
		 * @return -
		 */	
		IMPORT_C void SetPressedComponent(TCutterBarPressedIcon aComponentIndex);

    public:

		/**
		 * Handles key events from framework.
		 * 
		 * @param aKeyEvent  the key event
		 * @param aType  the type of key event
		 *
		 * @return always <code>EKeyWasNotConsumed</code>
		 */
		TKeyResponse OfferKeyEventL(const TKeyEvent& aKeyEvent,
			TEventCode aType);        

    private:

        /**
         * Default constructor.
		 *
         * @param aCont  pointer to the container
         */
        void ConstructL( const CCoeControl* aParent, TBool aDrawBorder );

       /**
        * From CoeControl,SizeChanged.
        */
        void SizeChanged();

       /**
        * From CoeControl,CountComponentControls.
		*
		* @return  number of component controls in this control
        */
        TInt CountComponentControls() const;

       /**
        * From CCoeControl,ComponentControl.
		*
		* @param aIndex  index of the control to return
        */
        CCoeControl* ComponentControl(TInt aIndex) const;

       /**
        * From CCoeControl,Draw.
		*
		* @param aRect  rectangle to draw
        */
        void Draw(const TRect& aRect) const;

       /**
        * From MCoeControlObserver, called when there is a control event
		* to handle.
		*
		* @param aControl  control originating the event
		* @param aEventType  event type
        */
        void HandleControlEventL(CCoeControl* aControl,TCoeEvent aEventType);
                
        // for test use
        void DrawCoordinate(TInt aX, TInt aY, TInt aData1, TInt aData2, const TDesC& aInfo) const;

		/**
		 * Calculates the rect of the slider area that the user 
		 * has selected to be cut
		 * 
		 * @param -
		 *
		 * @return rect of the area to be cut
		 */
        TRect CVeiCutterBar::CalculateCutAreaRect() const;

    private:

		/** In point. */
		TUint iInPoint;

		/** Out point. */
		TUint iOutPoint;

		TUint iTotalDuration;
		/** Current point. This is where the vertical bar is drawn. */
		TUint iCurrentPoint;

		/** Flag for marking when the editing is finished. */
		TBool iFinished;

		TRect iCutBarRect;
		TRect iScissorsIconRect;
		
		TBool iDimmed;
		TBool iDrawBorder;
		TBool iDrawPlayHead;
		
		/** Slider Graphics */
		CFbsBitmap*		iScissorsIcon;
		CFbsBitmap*		iScissorsIconMask;
		CFbsBitmap*		iSliderLeftEndIcon;
		CFbsBitmap*		iSliderLeftEndIconMask;
		CFbsBitmap*		iSliderMiddleIcon;
		CFbsBitmap*		iSliderMiddleIconMask;
		CFbsBitmap*		iSliderRightEndIcon;
		CFbsBitmap*		iSliderRightEndIconMask;		
		CFbsBitmap*		iSliderSelectedLeftEndIcon;
		CFbsBitmap*		iSliderSelectedLeftEndIconMask;
		CFbsBitmap*		iSliderSelectedMiddleIcon;
		CFbsBitmap*		iSliderSelectedMiddleIconMask;
		CFbsBitmap*		iSliderSelectedRightEndIcon;
		CFbsBitmap*		iSliderSelectedRightEndIconMask;		
		CFbsBitmap*		iPlayheadIcon;
		CFbsBitmap*		iPlayheadIconMask;
		CFbsBitmap*		iPlayheadIconPressed;
		CFbsBitmap*		iPlayheadIconPressedMask;
		CFbsBitmap*		iStartMarkIcon;
		CFbsBitmap*		iStartMarkIconMask;	
		CFbsBitmap*		iStartMarkIconPressed;
		CFbsBitmap*		iStartMarkIconPressedMask;	
		CFbsBitmap*		iEndMarkIcon;
		CFbsBitmap*		iEndMarkIconMask;
		CFbsBitmap*		iEndMarkIconPressed;
		CFbsBitmap*		iEndMarkIconPressedMask;		
		CFbsBitmap*		iCutAreaBorderIcon;
		CFbsBitmap*		iCutAreaBorderIconMask;		

		/** Rects for the slider graphics */
		TRect  iSliderLeftEndIconRect;
		TRect  iSliderRightEndIconRect;
		TRect  iSliderMiddleIconRect;
		TRect  iSliderSelectedLeftEndIconRect;
		TRect  iSliderSelectedMiddleIconRect;
		TRect  iSliderSelectedRightEndIconRect;
		TRect  iPlayheadIconRect;
		TRect  iCutAreaBorderIconRect;

		/** Start mark rect. Position moves dynamically. */
        TRect iStartMarkRect;

		/** End mark rect. Position moves dynamically. */        
        TRect iEndMarkRect;

        TRect iStartMarkTouchRect;
        TRect iEndMarkTouchRect;
		TRect iPlayheadTouchRect;
		
		// Tells what cutter bar component is pressed if any
		TCutterBarPressedIcon iPressedComponent;

    };
#endif

// End of File
