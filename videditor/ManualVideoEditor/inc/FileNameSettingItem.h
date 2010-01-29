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



#ifndef __FILENAMESETTINGITEM_H__
#define __FILENAMESETTINGITEM_H__

//  INCLUDES
#include <aknsettingitemlist.h>

// CONSTANTS
_LIT( KCharColon, ":" ); // Illegal character for filename.
const TText KCharDot = '.'; // Dot character.

// CLASS DECLARATION

/**
 * Custom setting item for filename. Filename validity is checked.
 */
class CFileNameSettingItem: public CAknTextSettingItem
{
public:
    // Constructor and destructor

    /**
     * Constructor. 
     *
     * @param aIdentifier Resource identifier for this setting item.
     * @param aText Setting text.
     * @param aIllegalFilenameTextResourceID Resource identifier for 
     *        warning note text.
     * @param aUnsuitableFilenameTextResourceID Resource identifier for
     *        warning note text.
     */
    CFileNameSettingItem( TInt aIdentifier, 
                          TDes& aText, 
                          TInt aIllegalFilenameTextResourceID, 
                          TInt aUnsuitableFilenameTextResourceID );

    /**
     * Destructor.
     */
    ~CFileNameSettingItem();

public:
    // Functions from base classes

    /**
     * From <code>MAknSettingPageObserver</code>, handles events reported
     * by the setting page.
     *
     * @param aSettingPage Notified setting page.
     * @param aEventType Occured event type.
     */
    void HandleSettingPageEventL( CAknSettingPage* aSettingPage,
                                 TAknSettingPageEvent aEventType );

    /**
     * From <code>CAknTextSettingItem</code>, this launches the setting
     * page for text editing.
     *
     * @param aCalledFromMenu Ignored in this and under laying
     *        <code>CAknTextSettingItem</code> class.
     */
    void EditItemL( TBool aCalledFromMenu );

private:
    // Data

    /**
     * The text in editor before editing is started.
     */
    HBufC* iTextBeforeEditing;

    /**
     * Indicates whether ok is pressed and the file name is incorrect.
     */
    TBool iInvalidFilenameOked;

    /**
     * Resource identifier for illegal file name string.
     */
    TInt iIllegalFilenameTextResourceID;

    /**
     * Resource identifier for unsuitable file name string.
     */
    TInt iUnsuitableFilenameTextResourceID;

};
#endif // __FILENAMESETTINGITEM_H__

// End of File
