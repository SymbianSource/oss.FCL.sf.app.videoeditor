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
#ifndef CMULTILINEQUERYDIALOG_H_
#define CMULTILINEQUERYDIALOG_H_

//  INCLUDES
#include <AknQueryDialog.h>

// CLASS DECLARATION

/**
 *  Text query dialog that accepts multiple lines input.
 *  Wrapper for CAknTextQueryDialog 
 */
class CMultiLineQueryDialog : public CAknTextQueryDialog
    {
    
    public:  // Constructors and destructor

        /**
         * Two-phased constructor.
         * @see CAknTextQueryDialog::NewL
         */
        IMPORT_C static CMultiLineQueryDialog* NewL( TDes& aDataText, 
                        const TTone& aTone = ENoTone );
        
        /**
         * Destructor.
         */
        ~CMultiLineQueryDialog();

    public: // Functions from base classes
        
        /**
         * Handle key events.
         * @see CAknTextQueryDialog
         */
        IMPORT_C TKeyResponse OfferKeyEventL( const TKeyEvent& aKeyEvent, 
                                     			TEventCode aType );
        
    private:

        /**
         * C++ default constructor.
         * @see CAknTextQueryDialog::CAknTextQueryDialog
         */
    	CMultiLineQueryDialog( TDes& aDataText, const TTone& aTone = ENoTone );
        
    private: // Data    
    	

    };

#endif /*CMULTILINEQUERYDIALOG_H_*/
