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


#include "FilenameSettingItem.h"
#include "manualvideoeditor.hrh"	// For setting item ids
#include "VeiSettingItemList.h"

#include <caknmemoryselectionsettingitem.h> 
#include <manualvideoeditor.rsg>

CVeiSettingItemList::CVeiSettingItemList( TVeiSettings& aSettings ): iSettings( aSettings )
{}

CVeiSettingItemList::~CVeiSettingItemList()
{}

/**
 * Called by framework when the view size is changed. Resizes the
 * setting list accordingly.
 */
void CVeiSettingItemList::SizeChanged()
    {
    if ( ListBox())
        {
        ListBox()->SetRect( Rect());
        }
    }

/**
 *
 *
 */
CAknSettingItem* CVeiSettingItemList::CreateSettingItemL( TInt aSettingId )
    {
    CAknSettingItem* settingItem = NULL;

    switch ( aSettingId )
        {
        /**
         * Default video name
         */
        case EVeiVideoNameSettingItem:
            settingItem = new( ELeave )CFileNameSettingItem( aSettingId, iSettings.DefaultVideoName(), R_VEI_ILLEGAL_FILENAME, R_VEI_UNSUITABLE_FILENAME );
            break;
            /**
             * Default snapshot name
             */
        case EVeiSnapshotNameSettingItem:
            settingItem = new( ELeave )CFileNameSettingItem( aSettingId, iSettings.DefaultSnapshotName(), R_VEI_ILLEGAL_FILENAME, R_VEI_UNSUITABLE_FILENAME );
            break;
            /**
             * Save quality
             */
        case EVeiSaveQualitySettingItem:
            settingItem = new( ELeave )CAknEnumeratedTextPopupSettingItem( aSettingId, iSettings.SaveQuality());
            break;
            /**
             * Memory in use
             */
        case EVeiMemoryInUseSettingItem:
            settingItem = new( ELeave )CAknMemorySelectionSettingItem( aSettingId, iSettings.MemoryInUse());
            break;
            /**
             * Default
             */
        default:
            // Panic the aplication if all setting items defined in rss
            // are not constructed
            User::Panic( _L( "CVeiSettingItemList" ), KErrNotFound );
            break;
        }
    // Return constructed item and transfer the ownership to base class.
    return settingItem;
    }

/**
 *
 *
 */
void CVeiSettingItemList::ChangeFocusedItemL()
    {
    EditItemL( ListBox()->CurrentItemIndex(), ETrue );
    }

/**
 * 
 *
 */
void CVeiSettingItemList::EditItemL( TInt aIndex, TBool aCalledFromMenu )
    {
    CAknSettingItemList::EditItemL( aIndex, aCalledFromMenu );
    ( *SettingItemArray())[aIndex]->StoreL();
    }

// End of File
