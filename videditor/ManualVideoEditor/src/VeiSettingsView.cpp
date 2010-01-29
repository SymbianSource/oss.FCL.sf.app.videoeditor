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
#include <aknviewappui.h>
#include <avkon.hrh>
#include <manualvideoeditor.rsg>
#include <akncontext.h> 
#include <akntitle.h> 
#include <barsread.h>
#include <s32stor.h>
#include <aknquerydialog.h> 
#include <s32stor.h> 

// User includes 
#include "Manualvideoeditor.hrh"
#include "VeiSettingsView.h"
#include "VeiSettingsContainer.h" 
#include "VeiApp.h" 
#include "VeiAppUi.h"



// ================= MEMBER FUNCTIONS =======================

CVeiSettingsView* CVeiSettingsView::NewL()
    {
    CVeiSettingsView* self = CVeiSettingsView::NewLC();
    CleanupStack::Pop( self );

    return self;
    }


CVeiSettingsView* CVeiSettingsView::NewLC()
    {
    CVeiSettingsView* self = new( ELeave )CVeiSettingsView();
    CleanupStack::PushL( self );
    self->ConstructL();

    return self;
    }


// ---------------------------------------------------------
// CVeiSettingsView::ConstructL(const TRect& aRect)
// EPOC two-phased constructor
// ---------------------------------------------------------
//
void CVeiSettingsView::ConstructL()
    {
    BaseConstructL( R_VEI_SETTINGS_VIEW );
    }


CVeiSettingsView::CVeiSettingsView()
    {
    }

// ---------------------------------------------------------
// CVeiSettingsView::~CVeiSettingsView()
// Destructor
// ---------------------------------------------------------
//
CVeiSettingsView::~CVeiSettingsView()
    {
    if ( iContainer )
        {
        AppUi()->RemoveFromViewStack( *this, iContainer );
        }

    delete iContainer;
    }

// ---------------------------------------------------------
// TUid CVeiSettingsView::Id()
// Returns settings view UID
// ---------------------------------------------------------
//
TUid CVeiSettingsView::Id()const
    {
    return TUid::Uid( EVeiSettingsView );
    }

// ---------------------------------------------------------
// CVeiSettingsView::HandleCommandL(TInt aCommand)
// ---------------------------------------------------------
//
void CVeiSettingsView::HandleCommandL( TInt aCommand )
    {
    switch ( aCommand )
        {
        /**
         * Back
         */
        case EAknSoftkeyBack:
            // Do not force 
            // the settings view into portrait, even though it contains text input.
            //AppUi()->SetOrientationL( iOriginalOrientation );
            STATIC_CAST( CVeiAppUi* , AppUi())->WriteSettingsL( iSettings );
            // Activate Edit Video view
            AppUi()->ActivateLocalViewL( TUid::Uid( EVeiEditVideoView ));
            break;
            /**
             * Change
             */
        case EVeiCmdSettingsViewChange:
            iContainer->ChangeFocusedItemL(); // Start editing the focused item.
            break;
            /**
             * Help
             */
        case EVeiCmdSettingsViewHelp:
            AppUi()->HandleCommandL( EVeiCmdSettingsViewHelp );
            break;
            /**
             * Exit
             */
        case EEikCmdExit:
            STATIC_CAST( CVeiAppUi* , AppUi())->WriteSettingsL( iSettings );
            AppUi()->HandleCommandL( EEikCmdExit );
            break;
        default:
            AppUi()->HandleCommandL( aCommand );
            break;
        }
    }

// ---------------------------------------------------------
// CVeiSettingsView::HandleClientRectChange()
// ---------------------------------------------------------
//
void CVeiSettingsView::HandleClientRectChange()
    {
    if ( iContainer )
        {
        iContainer->SetRect( ClientRect());
        }

    }

// ---------------------------------------------------------
// CVeiSettingsView::DoActivateL(...)
// ---------------------------------------------------------
//
void CVeiSettingsView::DoActivateL( const TVwsViewId& /*aPrevViewId*/, 
                                    TUid /*aCustomMessageId*/, 
                                    const TDesC8& /*aCustomMessage*/ )
    {
    // do not force the settings view into portrait,
    //  even though it contains text input.
    // iOriginalOrientation = AppUi()->Orientation();
    //AppUi()->SetOrientationL( CAknAppUiBase::EAppUiOrientationPortrait );

    if ( !iContainer )
        {
        STATIC_CAST( CVeiAppUi* , AppUi())->ReadSettingsL( iSettings );
        // Read da settings.

        iContainer = new( ELeave )CVeiSettingsContainer;
        iContainer->SetMopParent( this );
        iContainer->ConstructL( AppUi()->ClientRect(), iSettings );
        AppUi()->AddToStackL( *this, iContainer );
        }

    CEikStatusPane* statusPane = (( CAknAppUi* )iEikonEnv->EikAppUi())->StatusPane();

    CAknTitlePane* titlePane = ( CAknTitlePane* )statusPane->ControlL( TUid::Uid( EEikStatusPaneUidTitle ));
    TResourceReader reader1;
    iCoeEnv->CreateResourceReaderLC( reader1, R_VEI_SETTINGS_VIEW_TITLE_NAME );
    titlePane->SetFromResourceL( reader1 );
    CleanupStack::PopAndDestroy(); //reader1
    }

// ---------------------------------------------------------
// CVeiSettingsView::DoDeactivate()
// ---------------------------------------------------------
//
void CVeiSettingsView::DoDeactivate()
    {
    if ( iContainer )
        {
        AppUi()->RemoveFromViewStack( *this, iContainer );
        }

    delete iContainer;
    iContainer = NULL;
    }

// End of File
