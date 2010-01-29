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


#ifndef VEISETTINGSVIEW_H
#define VEISETTINGSVIEW_H

// INCLUDES
// System includes
#include <aknview.h>        // CAknView
// User includes
#include "VeiSettings.h"    // TVeiSettings

// FORWARD DECLARATIONS
class CVeiSettingsContainer;

// CLASS DECLARATION

/**
 * CVeiSettingsView view class.
 */
class CVeiSettingsView: public CAknView
{
public:
    // Constructors and destructor

    /**
     * Two-phased constructor.
     */
    static CVeiSettingsView* NewL();

    /**
     * Two-phased constructor.
     */
    static CVeiSettingsView* NewLC();

    /**
     * Destructor.
     */
    ~CVeiSettingsView();

public:
    // From CAknView

    /**
     * 
     */
    TUid Id()const;

    /**
     * 
     */
    void HandleCommandL( TInt aCommand );

    /**
     *  
     */
    void HandleClientRectChange();

private:
    // From CAknView

    /**
     * From AknView
     */
    void DoActivateL( const TVwsViewId& aPrevViewId, TUid aCustomMessageId,
                     const TDesC8& aCustomMessage );

    /**
     * From AknView
     */
    void DoDeactivate();

private:

    /**
     * C++ default constructor.
     */
    CVeiSettingsView();

    /**
     * Symbian 2nd phase constructor.
     */
    void ConstructL();

private:
    // Data

    /** 
     * Container. 
     */
    CVeiSettingsContainer* iContainer;

    /**
     * 
     */
    TVeiSettings iSettings;

    /**
     * Store the original orientation when forcing to portrait
     */
    //CAknAppUiBase::TAppUiOrientation iOriginalOrientation;
};

#endif 

// End of Files
