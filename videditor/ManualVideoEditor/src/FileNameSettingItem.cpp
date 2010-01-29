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

 

// INCLUDE FILES
#include "FileNameSettingItem.h"
#include <aknnotewrappers.h>
#include <stringloader.h>


CFileNameSettingItem::CFileNameSettingItem( TInt aIdentifier, TDes& aText,
										   TInt aIllegalFilenameTextResourceID,
										   TInt aUnsuitableFilenameTextResourceID )
    :CAknTextSettingItem( aIdentifier, aText ),
    iIllegalFilenameTextResourceID(aIllegalFilenameTextResourceID), 
    iUnsuitableFilenameTextResourceID(aUnsuitableFilenameTextResourceID)
    {
    }

CFileNameSettingItem::~CFileNameSettingItem()
    {
    if ( iTextBeforeEditing )
        {
        delete iTextBeforeEditing;
        }
    }

void CFileNameSettingItem::EditItemL( TBool aCalledFromMenu )
    {
    if ( !iInvalidFilenameOked )
        {
        // Delete old buffer if allocated
        if ( iTextBeforeEditing )
            {
            delete iTextBeforeEditing;
            iTextBeforeEditing = NULL;
            }
        // Save the value before editing it
        iTextBeforeEditing = HBufC::NewL( SettingTextL().Length());
        iTextBeforeEditing->Des().Copy( SettingTextL());
        }
    CAknTextSettingItem::EditItemL( aCalledFromMenu );
    }

void CFileNameSettingItem::HandleSettingPageEventL( 
                                CAknSettingPage* aSettingPage, 
                                TAknSettingPageEvent aEventType ) 
    {

    switch ( aEventType )
        {
        /**
         * Cancel event.
         */
        case EEventSettingCancelled:
                {
                if ( iInvalidFilenameOked )
                    {
                    iInvalidFilenameOked = EFalse; // Reset invalid filename flag

                    TPtr internalText = InternalTextPtr();
                    internalText.Delete( 0, internalText.Length());
                    internalText.Append( *iTextBeforeEditing );
                    StoreL();
                    LoadL();
                    }
                break;
                }
            /**
             * Change event.
             */
        case EEventSettingChanged:
            break;
            /**
             * Ok event.
             */
        case EEventSettingOked:
                {
                RFs fileSystem;

                CleanupClosePushL( fileSystem );
                User::LeaveIfError( fileSystem.Connect());

                TText illegalCharacter;

                if ( !fileSystem.IsValidName( SettingTextL(), illegalCharacter ) )
                    {
                    iInvalidFilenameOked = ETrue;

                    HBufC* noteText;

                    // If dot keyed
                    if ( illegalCharacter == KCharDot )
                        {
                        noteText = StringLoader::LoadLC( iUnsuitableFilenameTextResourceID );
                        }
                    else
                        {
                        noteText = StringLoader::LoadLC( iIllegalFilenameTextResourceID );
                        }

                    CAknWarningNote* note = new( ELeave )CAknWarningNote( ETrue );

                    note->ExecuteLD( *noteText );
                    CleanupStack::PopAndDestroy( noteText );

                    EditItemL( EFalse ); // Start editing the text again.
                    }
                else if ( SettingTextL().Find( KCharColon ) == 1 )
                    {
                    iInvalidFilenameOked = ETrue;

                    // Load note text from resources.
                    HBufC* noteText = StringLoader::LoadLC( iIllegalFilenameTextResourceID );
                        

                    CAknWarningNote* note = new( ELeave )CAknWarningNote( ETrue );
                    note->ExecuteLD( *noteText );

                    CleanupStack::PopAndDestroy( noteText ); // Pop and destroy.

                    EditItemL( EFalse ); // Start editing the text again.
                    }
                else
                    {
                    // Do nothing.
                    }

                CleanupStack::PopAndDestroy( &fileSystem ); 
                break;
                }
        }
    // Super class handles events.
    CAknTextSettingItem::HandleSettingPageEventL( aSettingPage, aEventType );

    }
// End of File
