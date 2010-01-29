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



#ifndef VEDSIMPLECUTVIDEOAPP_H
#define VEDSIMPLECUTVIDEOAPP_H

// INCLUDES
// System includes
#include <aknapp.h>

// CLASS DECLARATION

/**
 * CVeiApp application class.
 * Provides factory to create concrete document object.
 */
class CVeiSimpleCutVideoApp : public CAknApplication
    {
    
    public: // Functions from base classes
    private:

        /**
        * From CApaApplication, creates CVeiDocument document object.
        * @return A pointer to the created document object.
        */
        CApaDocument* CreateDocumentL();
        
        /**
        * From CApaApplication, returns application's UID (KUidveijo).
        * @return The value of KUidveijo.
        */
        TUid AppDllUid() const;

		CDictionaryStore* OpenIniFileLC(RFs& aFs) const; 

    };

#endif

// End of File

