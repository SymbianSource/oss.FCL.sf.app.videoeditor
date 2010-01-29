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


/*	
*   File:       VeiSlider.h
*   Created:    30-10-2004
*   Author:     
*               
*/

#ifndef __VEISLIDER_H__
#define __VEISLIDER_H__

#include <coecntrl.h>

class CFbsBitmap;

/*! 
  @class CVeiSlider
  @discussion Simple slider control base class.
  */
  
class CVeiSlider : public CCoeControl
    {
	public: 

		/*!
		  @function ~CVeiSlider
		  @discussion Destroy the object and release all memory objects
		  */
		IMPORT_C ~CVeiSlider();

	public: // new functions

		/*!
		  @function SetMinimum
		  @param aValue the new minimum value
		  @discussion Sets the minimum value of the slider
		  */
		IMPORT_C void SetMinimum(TInt aValue);

		/*!
		  @function SetMaximum
		  @param aValue the new maximum value
		  @discussion Sets the maximum value of the slider
		  */
		IMPORT_C void SetMaximum(TInt aValue);

		/*!
		  @function SetStepL
		  @param aValue the new step value
		  @discussion Sets the step of the slider
		  */
		IMPORT_C void SetStep(TUint aValue);

		/*!
		  @function SetStepAmount
		  @param aValue the new step amount
		  @discussion Sets the number of steps in the slider
		  */
		IMPORT_C void SetStepAmount(TUint8 aValue);

		/*!
		  @function SetPosition
		  @discussion Sets the position of the slider. Panics if the position is out of bounds.
		  */
		IMPORT_C void SetPosition(TInt aValue);

		/*!
		  @function Minimum
		  @discussion Gets the minimum value of the slider
		  @return minimum value
		  */
		IMPORT_C TInt Minimum() const;

		/*!
		  @function Maximum
		  @discussion Gets the maximum value of the slider
		  @return maximum value
		  */
		IMPORT_C TInt Maximum() const;

		/*!
		  @function Step
		  @discussion Gets the step of the slider
		  @return current step
		  */
		IMPORT_C TInt Step() const;

		/*!
		  @function SliderPosition
		  @discussion Gets the position of the slider
		  @return current position
		  */
		IMPORT_C TInt SliderPosition() const;

		/*!
		  @function Increment
		  @discussion Increments the slider
		  */
		IMPORT_C void Increment();

		/*!
		  @function Decrement
		  @discussion Decrements the slider
		  */
		IMPORT_C void Decrement();
		
		/*!
		  @function MinimumSize
		  @discussion Gets the minimum size of this component
		  @return a minimum size of the control
		  */
		virtual TSize MinimumSize() = 0;

	protected:

		/*!
		  @function CVeiSlider
		  @discussion Constructs this object
		  */
		CVeiSlider();				

		/*!
		  @function LoadBitmapL
		  @discussion Loads one bitmap and its mask
		  */
		void LoadBitmapL( 
            CFbsBitmap*& aBitmap, 
            CFbsBitmap*& aMask, 
            TInt aBitmapIndex, 
            TInt aMaskIndex 
            ) const;

	public: // from CoeControl

		/*!
		  @function CountComponentControls
		  @return Number of component controls 
		  */
		virtual TInt CountComponentControls() const;

		/*!
		  @function ComponentControl.
		  @param aIndex index of the component control
		  @return Pointer to component control
		  */
		virtual CCoeControl* ComponentControl(TInt aIndex) const;


	protected: // data

		/// bitmap holding the slider background
		CFbsBitmap* iSliderBg;

		/// mask for the slider background
		CFbsBitmap* iSliderBgMask;

		/// bitmap holding the slider tab that moves
		CFbsBitmap* iSliderTab;

		/// mask for the slider tab
		CFbsBitmap* iSliderTabMask;

	private: // data

		/// minimum value of the slider
		TInt iMinimumValue;

		/// maximum value of the slider
		TInt iMaximumValue;

		/// step value
		TUint iStep;

		/// number of steps
		TUint8 iNumberOfSteps;

		/// current position
		TInt iPosition;

    };



/*! 
  @class CVeiVerticalSlider
  @discussion Vertical slider control
  */

NONSHARABLE_CLASS( CVeiVerticalSlider ) : public CVeiSlider
    {
	public: 

		/*!
		  @function NewL
		  @discussion Create a CVeiVerticalSlider object, which will draw itself to aRect
		  @param aRect the rectangle this view will be drawn to
		  @return a pointer to the created instance of CVeiVerticalSlider
		  */
		IMPORT_C static CVeiVerticalSlider* NewL(const TRect& aRect, const CCoeControl& aControl);

		/*!
		  @function NewLC
		  @discussion Create a CVeiSlider object, which will draw itself to aRect
		  @param aRect the rectangle this view will be drawn to
		  @return a pointer to the created instance of CVeiSlider
		  */
		IMPORT_C static CVeiVerticalSlider* NewLC(const TRect& aRect, const CCoeControl& aControl);

		/*!
		  @function ~CVeiSlider
		  @discussion Destroy the object and release all memory objects
		  */
		IMPORT_C ~CVeiVerticalSlider();

		/*!
		  @function MinimumSize
		  @discussion Gets the minimum size of this component
		  @return a minimum size of the control
		  */
		IMPORT_C TSize MinimumSize();

	private:

		/*!
		  @fuction ConstructL
		  @discussion Perform the second phase construction of a CVeiVerticalSlider object
		  @param aRect Frame rectangle for container.
		  */
		void ConstructL(const TRect& aRect, const CCoeControl& aControl);

		/*!
		  @function CVeiVerticalSlider
		  @discussion Constructs this object
		  */
		CVeiVerticalSlider();

	public: // from CoeControl

		/*!
		  @function Draw
		  @discussion Draw this CVeiVerticalSlider to the screen
		  @param aRect the rectangle of this view that needs updating
		  */
		virtual void Draw(const TRect& aRect) const;

		/*!
		  @function SizeChanged
		  @discussion Responds to changes to the size and position of the contents of this control.
		  */
		virtual void SizeChanged();

    };



/*! 
  @class CVeiHorizontalSlider
  @discussion Horizontal slider control
  */

NONSHARABLE_CLASS( CVeiHorizontalSlider ) : public CVeiSlider
    {
	public: 

		/*!
		  @function NewL
		  @discussion Create a CVeiHorizontalSlider object, which will draw itself to aRect
		  @param aRect the rectangle this view will be drawn to
		  @return a pointer to the created instance of CVeiHorizontalSlider
		  */
		IMPORT_C static CVeiHorizontalSlider* NewL(const TRect& aRect, const CCoeControl& aControl);

		/*!
		  @function NewLC
		  @discussion Create a CVeiSlider object, which will draw itself to aRect
		  @param aRect the rectangle this view will be drawn to
		  @return a pointer to the created instance of CVeiSlider
		  */
		IMPORT_C static CVeiHorizontalSlider* NewLC(const TRect& aRect, const CCoeControl& aControl);

		/*!
		  @function ~CVeiSlider
		  @discussion Destroy the object and release all memory objects
		  */
		IMPORT_C ~CVeiHorizontalSlider();

		/*!
		  @function MinimumSize
		  @discussion Gets the minimum size of this component
		  @return a minimum size of the control
		  */
		IMPORT_C TSize MinimumSize();

	private:

		/*!
		  @fuction ConstructL
		  @discussion Perform the second phase construction of a CVeiHorizontalSlider object
		  @param aRect Frame rectangle for container.
		  */
		void ConstructL(const TRect& aRect, const CCoeControl& aControl);

		/*!
		  @function CVeiHorizontalSlider
		  @discussion Constructs this object
		  */
		CVeiHorizontalSlider();

	public: // from CoeControl

		/*!
		  @function Draw
		  @discussion Draw this CVeiHorizontalSlider to the screen
		  @param aRect the rectangle of this view that needs updating
		  */
		virtual void Draw(const TRect& aRect) const;

		/*!
		  @function SizeChanged
		  @discussion Responds to changes to the size and position of the contents of this control.
		  */
		virtual void SizeChanged();

    };

#endif // __VEISLIDER_H__

// End of File
