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
*   File:       ExtProgressDialog.h
*   Created:    17-10-2005
*   Author:     
*               
*/

#ifndef EXTPROGRESSDIALOG_H
#define EXTPROGRESSDIALOG_H

#include <akndialog.h>
#include <coecobs.h>
#include <ConeResLoader.h> 

// Forward Declarations
class CFbsBitmap;
class CEikLabel;
class CEikProgressInfo;
class CExtProgressContainer;
class CExtProgressNoteAnimationControl;

/*  CLASS: MExtProgressDialogCallback
*
*
*/ 
class MExtProgressDialogCallback
{
public:

    /** DialogDismissedL
    *
    * Callback method. Gets called when a dialog is dismissed.
    *
    *   @param aButtonId - id of the button pressed
    *   @return -
    */
    virtual void DialogDismissedL( TInt aButtonId ) = 0;

};

/*  CLASS: CExtProgressDialog
*
*
*   Usage:
*      
*     iProgNote = new (ELeave) CExtProgressDialog (&iProgNote, iBitmap1, iBitmap2);
*     iProgNote->PrepareLC(R_WAIT_DIALOG);
*     iProgNote->GetProgressInfoL()->SetFinalValue (aFinalValue);
*     iProgNote->StartAnimationL();
*     iProgNote->SetTextL( aPrompt );
*     iProgNote->SetCallback (this);    
*     iProgNote->RunLD();
*
*   Resource definition:
*
*     RESOURCE DIALOG r_wait_dialog
*     {
*        flags = EAknWaitNoteFlags;
*        buttons = R_AVKON_SOFTKEYS_CANCEL;
*     }
*
*/ 
class CExtProgressDialog :	public CAknDialog
{

public:

  	/** Constructor
	*
	*	@param aBitmap - background bitmap
	*	@param aSelectedItem - selected item
	*	@param aItems - Plugin info item array
	*	@return -
	*/
    IMPORT_C CExtProgressDialog(CExtProgressDialog** aSelfPtr);

  	/** Destructor
	*
	*	@param  -
	*	@return -
	*/
    IMPORT_C ~CExtProgressDialog();

   	/** PrepareLC
	*
	*	@param aResourceId - resource id
	*	@return -
	*/
    IMPORT_C void PrepareLC(TInt aResourceId);	
	
   	/** SetCallback
	*
	*	@param aCallback - callback
	*	@return -
	*/
    IMPORT_C void SetCallback(MExtProgressDialogCallback* aCallback);

   	/** GetProgressInfoL
	*
	*	@param - 
	*	@return - progress info 
	*/
    IMPORT_C CEikProgressInfo* GetProgressInfoL();

   	/** StartAnimationL
	*
	*	@param - 
	*	@return - 
	*/
    IMPORT_C void StartAnimationL();
    
   	/** SetTextL
	*
	*	@param aText - title text
	*	@return - 
	*/
    IMPORT_C void SetTextL(const TDesC &aText);
    
    /** SetAnimationResourceId
	*
	*	@param aResourceId - animation resource id
	*	@return - 
	*/
    IMPORT_C void SetAnimationResourceIdL(const TInt &aResourceId);
 
protected:

    /** OkToExitL
    * 
    * From CEikDialog update member variables .
    * @param aButtonId The ID of the button that was activated.
    * @return Should return ETrue if the dialog should exit,
    *    and EFalse if it should not
    */
    TBool OkToExitL( TInt aButtonId );

    /** OfferKeyEventL
    *  
    * @see CCoeControl
    *
    */
    TKeyResponse OfferKeyEventL(const TKeyEvent& aKeyEvent,TEventCode aType);

    /** HandleControlEventL
    *  
    * @see CCoeControl
    *
    */
    void HandleControlEventL(CCoeControl* /*aControl*/,TCoeEvent aEventType);

    /** PreLayoutDynInitL
    *  
    * @see CEikDialog
    *
    */
    void PreLayoutDynInitL();

    /** SetSizeAndPosition
    *  
    * @see CEikDialog
    *
    */
    void SetSizeAndPosition(const TSize &aSize);
    
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
  
private:
    
    CExtProgressDialog** 		iSelfPtr;

    MExtProgressDialogCallback* iCallback;
    RConeResourceLoader 		iResLoader;
    
    CExtProgressContainer* 		iContainer;
};

#endif

// End of File
