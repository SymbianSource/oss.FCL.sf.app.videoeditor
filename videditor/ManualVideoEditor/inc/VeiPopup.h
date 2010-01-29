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


#ifndef VEIPOPUP_H
#define VEIPOPUP_H

//  INCLUDES
//#include <aknview.h>

//#include <utility.h>

const TInt KAmountOfMenuItems = 9;

class CVeiEditVideoView;

//  CLASS DECLARATION

/**
 *  CVeiPopup view class.
 */
class CVeiPopup: public CBase 

{
public:
    //Constructors and destructor

    /**
     * Static factory constructor.
     *
     * @param aView Instance of  video view.
     * @return Created <code>CVeiPopup</code> instance.
     */

    static CVeiPopup* NewL( CVeiEditVideoView& aView );


    /**
     * Static factory constructor. Leaves the created object in the
     * cleanup stack.
     *
     * @param aView Instance of video view.
     * @return Created <code>CVeiPopup</code> instance.
     */

    static CVeiPopup* NewLC( CVeiEditVideoView& aView );

    /**
     * Destructor.
     */
    virtual ~CVeiPopup();


public:
    // New functions

    /**
     * Opens insert audio popup list.
     */
    void ShowInsertAudioPopupList();

    /**
     * Opens video/image/text popup list.
     */
    void ShowInsertStuffPopupList();

    /**
     * Opens insert text popup list.
     */
    void ShowInsertTextPopupList();

    /**
     * Opens edit video popup list.
     */
    void ShowEditVideoPopupList();

    /**
     * Opens edit text popup list.
     */
    void ShowEditTextPopupList();

    /**
     * Opens edit image popup list.
     */
    void ShowEditImagePopupList();

    /**
     * Opens edit text style popup list where text style can be selescted.
     */
    void ShowEditTextStylePopUpList();

    /**
     * Opens edit audio popup list.
     */
    void ShowEditAudioPopupList();

    /**
     * Executes a popup list menu where end transition can be selected.
     */
    void ShowEndTransitionPopupListL();

    /**
     * Executes a popup list menu where middle transition can be selected.
     */
    void ShowMiddleTransitionPopupListL();

    /**
     * Executes a popup list menu where start transition can be selected.
     */
    void ShowStartTransitionPopupListL();

    /**
     * Opens effect selection popup list.
     */
    void ShowEffectSelectionPopupListL();

    /**
     * Shows the color selector dialog.
     */
    TBool ShowColorSelectorL( TRgb& aColor )const;

    /**
     * Shows the background selection dialog.
     */
    TInt ShowTitleScreenBackgroundSelectionPopupL( TBool& aImageSelected )const;

    /**
     * Opens insert text popup list.
     */
    void ShowTitleScreenStyleSelectionPopupL();


protected:
    // New functions

    /**
     * Shows a popup list with given parameters.
     *
     * @param aSoftkeysResourceId Softkeys id.
     * @param aPopupTitleResourceId Title for popup.
     * @param aArrayResourceId Array for items.
     * @param aTablesize Index of removed item (from array).
     * @param aDynPopup is popup dynamic or not.
     * @return Returns the index of selected item.
     */
    TInt ExecutePopupListL( TInt aSoftkeysResourceId, 
                            TInt aPopupTitleResourceId,
                            TInt aArrayResourceId,
                            TInt aTablesize,
                            TBool aDynPopup ) const;
private:
    /**
     * Symbian 2nd phase constructor.
     */
    void ConstructL();

    CVeiPopup( CVeiEditVideoView& aView );


private:
    // Data
    CVeiEditVideoView& iView;
    TInt RemoveArrayIndex[KAmountOfMenuItems];

};

#endif // VEIPOPUP_H

// End of File
