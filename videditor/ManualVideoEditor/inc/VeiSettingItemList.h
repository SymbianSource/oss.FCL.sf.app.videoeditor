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



#ifndef VEISETTINGITEMLIST_H
#define VEISETTINGSITEMLIST_H

// INCLUDES
#include <aknsettingitemlist.h> 
#include "VeiSettings.h"

//  CLASS DECLARATION

/**
 * Setting item list for setting items.
 */
class CVeiSettingItemList: public CAknSettingItemList
{
public:
    // Constructor and destructor

    /**
     * C++ default constructor for constructing this object.
     *
     * @param aSettings Reference to settings data class.
     */
    CVeiSettingItemList( TVeiSettings& aSettings );

    /**
     * Destroys the object and releases all memory objects.
     */
    ~CVeiSettingItemList();

public:
    // New functions

    /**
     * This launches the setting page for the highlighted item by calling
     * <code>EditItemL</code> on it.
     */
    void ChangeFocusedItemL();

public:
    // Functions from base classes

    /**
     * From <code>CAknSettingItemList</code>, this launches the setting
     * page for the current item by calling <code>EditItemL</code> on it.
     * Corresponding value is also saved by calling it's 
     * <code>StoreL</code> function.
     *
     * @param aIndex Current item's (Visible) index in the list.
     * @param aCalledFromMenu In this case, passed directly to the base
     *                        class.
     */
    void EditItemL( TInt aIndex, TBool aCalledFromMenu );

protected:
    // Functions from base classes

    /**
     * From <code>CAknSettingItemList</code>, framework method to create
     * a setting item based upon the user id <code>aSettingId</code>. 
     * Implementation decides what type to contruct.
     *
     * @param aSettingId ID to use to determine the type of the setting item.
     * @return A constructed (not 2nd-stage constructed) setting item.
     */
    CAknSettingItem* CreateSettingItemL( TInt aSettingId );

    /**
     * From <code>CCoeControl</code>, responds to size changes to sets the
     * size and position of the contents of this control.Resizes the
     * setting list accordingly.
     */
    void SizeChanged();

private:
    // Data

    /**
     * Application settings data.
     */
    TVeiSettings& iSettings;

};

#endif // VEISETTINGSITEMLIST_H

// End of File
