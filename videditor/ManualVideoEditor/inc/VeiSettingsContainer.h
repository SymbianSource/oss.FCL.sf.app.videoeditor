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



#ifndef VEISETTINGSCONTAINER_H
#define VEISETTINGSCONTAINER_H

// INCLUDES
#include <coecntrl.h>

// CLASS DECLARATION
class CVeiSettingItemList;
class TVeiSettings;

/**
 * CVeiSettingsContainer container control class.
 */
class CVeiSettingsContainer: public CCoeControl
{
public:
    // Constructors and destructor

    /**
     * Default constructor.
     *
     * @param aRect Frame rectangle for container.
     * @param aSettings Reference to application settings.
     */
    void ConstructL( const TRect& aRect, TVeiSettings& aSettings );

    /**
     * Destructor.
     */
    virtual ~CVeiSettingsContainer();

public:
    // New functions

    /**
     * This launches the setting page for the highlighted item by calling
     * <code>EditItemL</code> on it. The responsibility is handled to
     * <code>CVeiSettingItemList</code>.
     */
    void ChangeFocusedItemL();

private:
    // From CCoeControl

    /**
     * From <code>CCoeControl</code>, gets the specified component of 
     * a compound control.
     *
     * @param aIndex The index of the control to get.
     * @return The component control with an index of <code>aIndex</code>.
     */
    CCoeControl* ComponentControl( TInt aIndex )const;

    /**
     * From <code>CCoeControl</code>, gets the number of controls contained
     * in a compound control. 
     *
     * @return The number of component controls contained by this control.
     */
    TInt CountComponentControls()const;

    /**
     * From <code>CCoeControl</code>, gets the control's help context. 
     * Associates this control with a particular Help file and topic in
     * a context sensitive application. Sets public data members of
     * <code>aContext</code> to the required Help file UID (iMajor) and
     * context descriptor (iContext).
     *
     * @param aContext The control's help context.
     */
    void GetHelpContext( TCoeHelpContext& aContext )const;

    /**
     * From <code>CoeControl</code>, handles key events by passing them to
     * <code>CVeiSettingItemList</code>.
     *
     * @param aKeyEvent The key event.
     * @param aType The type of key event: <code>EEventKey</code>,
     *              <code>EEventKeyUp</code> or <code>EEventKeyDown</code>.
     * @return Indicates whether or not the key event was used by this
     *         control.
     */
    TKeyResponse OfferKeyEventL( const TKeyEvent& aKeyEvent, TEventCode aType );

    void HandleResourceChange( TInt aType );

    void SizeChanged();

private:
    // Data

    /**
     * Setting item list for handling all the setting items.
     */
    CVeiSettingItemList* iSettingItemList;

};

#endif // VEISETTINGSCONTAINER_H

// End of File
