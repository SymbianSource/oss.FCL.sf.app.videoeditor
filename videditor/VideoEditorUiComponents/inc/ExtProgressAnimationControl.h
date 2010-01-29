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
*   File:       ExtProgressAnimationControl.h
*   Created:    17-10-2005
*   Author:     
*               
*/

#ifndef EXTPROGRESSANIMATIONCONTROL_H
#define EXTPROGRESSANIMATIONCONTROL_H

// INCLUDES
#include <coecntrl.h>
   
// FORWARD DECLARATIONS
class CAknBitmapAnimation;
class CAknsBasicBackgroundControlContext;

// CLASS DECLARATION

/**	CLASS: CExtProgressAnimationControl
*
*	CExtProgressAnimationControl represent animated transition using two 
*	thumbnail images. The thumbnails and transition type can be set
*	to the control. 
*/
NONSHARABLE_CLASS( CExtProgressAnimationControl ) : public CCoeControl, public MCoeControlObserver
{

public:

/** @name Methods:*/
//@{
	/** NewL factory method, pops cleanup stack
	*
	*	@param aRect - control rectangle
	*	@param aParent - pointer to window owning control
	*	@return pointer to created CExtProgressAnimationControl object
	*/
	static CExtProgressAnimationControl * NewL (
		const TRect &		aRect,
		const CCoeControl *	aParent
		);

    /**	Destructor
	*
    *	@param -
	*	@return -
    */
    ~CExtProgressAnimationControl();

    /**	StartAnimationL
	*
	*	Starts animation routine.
	*
    *	@param - 
	*	@return -
    */
    void StartAnimationL( TInt aFrameIntervalInMilliSeconds=-1 );

    /**	SetAnimationResourceId
	*
    *	@param -
	*	@return -
    */
	void SetAnimationResourceId(const TInt &aResourceId);

    /**	MinimumSize
	*
    *	@param -
	*	@return -
    */
    //TSize MinimumSize();

    /**	SetAnimationResourceId
	*
    *	@param -
	*	@return -
    */
    void HandleControlEventL(CCoeControl* /*aControl*/,TCoeEvent /*aEventType*/);

    /**	StopAnimation
	*
    *	@param -
	*	@return -
    */
	void StopAnimation(); 

    /**	SetFrameIntervalL
	*
    *	@param -
	*	@return -
    */
	void SetFrameIntervalL(TInt aFrameIntervalInMilliSeconds);

//@}


private:

/** @name Methods:*/
//@{

    /**	Default constructor
	*
    *	@param -
	*	@return -
    */
    CExtProgressAnimationControl();

    /**	ConstructL
	*
	*	Second phase constructor
	*
	*	@param aRect - control rectangle
	*	@param aParent - pointer to window owning control
	*	@param aLeft - left icon
	*	@param aRight - right icon
	*	@return -
    */
    void ConstructL (
		const TRect &		aRect,
		const CCoeControl *	aParent
		);

	/**	SizeChanged
	*
    *	@see CCoeControl
    */
    void SizeChanged();

	/**	CountComponentControls
	*
    *	@see CCoeControl
    */
    //TInt CountComponentControls() const;

	/**	ComponentControl
	*
    *	@see CCoeControl
    */
    //CCoeControl * ComponentControl (TInt aIndex) const;

	/**	Draw
	*
    *	@see CCoeControl
    */
    void Draw (const TRect& aRect) const;
    
	/**
	* From CoeControl, MopSupplyObject.
	*
	* @param aId  
	*/
	virtual TTypeUid::Ptr MopSupplyObject( TTypeUid aId );

//@}
    
/** @name Members:*/
//@{

//@}
        
/** @name Members:*/
//@{

	TBool					iAnimationOn;        
	CAknBitmapAnimation*	iAnimation;
	TInt					iAnimationSpeedInMilliSeconds;
	TInt					iBorderWidth;
	
	TInt 					iAnimationResourceId;
	
	/** Background context. Skin stuff. */
	CAknsBasicBackgroundControlContext*	iBgContext;

//@}

};

#endif

// End of File
