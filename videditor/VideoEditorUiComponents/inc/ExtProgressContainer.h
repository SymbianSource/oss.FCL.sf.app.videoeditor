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
*   File:       CExtProgressContainer.h
*   Created:    17-10-2005
*   Author:     
*               
*/

#ifndef EXTPROGRESSCONTAINER_H
#define EXTPROGRESSCONTAINER_H

// INCLUDES
#include <coecntrl.h>
#include <akncontrol.h>

// FORWARD DECLARATIONS
class CEikProgressInfo;
class CExtProgressAnimationControl;
class CFbsBitmap;
class CEikLabel;
class CAknsBasicBackgroundControlContext;

/*  CLASS: CExtProgressContainer
*
*
*/ 
NONSHARABLE_CLASS( CExtProgressContainer ) :	public CCoeControl, public MCoeControlObserver
{

public:

	/** NewL factory method, does not pop cleanupstack
	*
	*   @param -
	*	@return pointer to created CExtProgressContainer object
	*/
    static CExtProgressContainer * NewL (const TRect& aRect, 
                                          CCoeControl* aParent);

	/** Destructor
	*
	*	@param -
	*	@return -
	*/
	virtual ~CExtProgressContainer ();

	/*	Second phase constructor	
	*
	*	@param -
	*	@return -
	*/
    void ConstructL (const TRect& aRect, CCoeControl* aParent);

	/*	GetProgressInfoL	
	*
	*	@param -
	*	@return -
	*/
    CEikProgressInfo* GetProgressInfoL();

	/*	GetAnimationControlL	
	*
	*	@param -
	*	@return -
	*/
    CExtProgressAnimationControl* GetAnimationControlL();
    
	/*	SetTextL	
	*
	*	@param aText - label text
	*	@return -
	*/
    void SetTextL(const TDesC &aText);

    // Test function
    void Test();
   
 
    
protected:

    /** CountComponentControls
    *  
    * @see CCoeControl
    *
    */
    TInt CountComponentControls() const;

    /** ComponentControl
    *  
    * @see CCoeControl
    *
    */
    CCoeControl* ComponentControl(TInt aIndex) const;

    /** SizeChanged
    *  
    * @see CCoeControl
    *
    */
    void SizeChanged();

    /** HandleControlEventL
    *  
    * @see CCoeControl
    *
    */
    void HandleControlEventL(CCoeControl* /*aControl*/,TCoeEvent aEventType);

    /** Draw
    * 
    * 
    * @see CCoeControl
    *
    */
    void Draw(const TRect& aRect) const;

	/**
	* From CoeControl, MopSupplyObject.
	*
	* @param aId  
	*/
	virtual TTypeUid::Ptr MopSupplyObject( TTypeUid aId );

    /** MinimumSize
    * 
    * 
    * @see CCoeControl
    *
    */
    TSize MinimumSize();

private:

    /** Default constructor, cannot leave.
	*
	*	@param -
	*	@return -
	*/
	CExtProgressContainer ();

private: // data

    CEikLabel*                      iLabel;
    CEikProgressInfo*               iProgressInfo;
    CExtProgressAnimationControl*   iAnimationControl;
    
	/** Background context. Skin stuff. */
	CAknsBasicBackgroundControlContext*	iBgContext;
};

#endif

// End of File
