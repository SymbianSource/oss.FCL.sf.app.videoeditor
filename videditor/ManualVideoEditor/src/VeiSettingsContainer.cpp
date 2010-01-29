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



// INCLUDES
// System includes
#include <manualvideoeditor.rsg>
// User includes
#include "VeiSettingsContainer.h"
#include "VeiSettingItemList.h"
#include "VideoEditorCommon.h"      // Help (application) UID
#include "VideoEditorHelp.hlp.hrh"  // Topic context (literal)
#include "VideoEditorDebugUtils.h"


// ================= MEMBER FUNCTIONS =======================

// ---------------------------------------------------------
// CVeiSettingsContainer::ConstructL(const TRect& aRect)
// EPOC two phased constructor
// ---------------------------------------------------------
//
void CVeiSettingsContainer::ConstructL( const TRect& aRect, TVeiSettings& aSettings )
    {
    CreateWindowL();
    SetRect( aRect );

    iSettingItemList = new( ELeave )CVeiSettingItemList( aSettings );
    iSettingItemList->SetMopParent( this );
    iSettingItemList->ConstructFromResourceL( R_VEI_SETTING_ITEM_LIST );


    ActivateL();
    }

// Destructor
CVeiSettingsContainer::~CVeiSettingsContainer()
    {
    delete iSettingItemList;
    }

// ---------------------------------------------------------
// CVeiSettingsContainer::CountComponentControls() const
// return nbr of controls inside this container
// ---------------------------------------------------------
//
TInt CVeiSettingsContainer::CountComponentControls()const
    {
    return 1;
    }

// ---------------------------------------------------------
// CVeiSettingsContainer::ComponentControl(TInt aIndex) const
// ---------------------------------------------------------
//
CCoeControl* CVeiSettingsContainer::ComponentControl( TInt aIndex )const
    {
    switch ( aIndex )
        {
        case 0:
            return iSettingItemList;
        default:
            return NULL;
        }
    }

// ----------------------------------------------------------------------------
// CVeiSettingsContainer::GetHelpContext( TCoeHelpContext& aContext ) const
//
//
// ----------------------------------------------------------------------------
//
void CVeiSettingsContainer::GetHelpContext( TCoeHelpContext& aContext )const
    {
    // Set UID of the CS Help file (same as application UID).
    aContext.iMajor = KUidVideoEditor;

    // Set the context.
    aContext.iContext = KVED_HLP_SETTINGS_VIEW;
    }


void CVeiSettingsContainer::ChangeFocusedItemL()
    {
    if ( iSettingItemList )
        {
        iSettingItemList->ChangeFocusedItemL();
        }
    }


// ---------------------------------------------------------
// CVeiSettingsContainer::OfferKeyEventL( 
//      const TKeyEvent& aKeyEvent,TEventCode aType )
// ---------------------------------------------------------
//

TKeyResponse CVeiSettingsContainer::OfferKeyEventL( const TKeyEvent& aKeyEvent, TEventCode aType )
    {
    if ( iSettingItemList )
        {
        return iSettingItemList->OfferKeyEventL( aKeyEvent, aType );
        }
    else
        {
        return EKeyWasNotConsumed;
        }
    }

void CVeiSettingsContainer::HandleResourceChange( TInt aType )
    {
    LOGFMT( KVideoEditorLogFile, "CVeiSettingsContainer::HandleResourceChange() In, aType:%d", aType );

    if ( aType == KEikDynamicLayoutVariantSwitch && OwnsWindow())
        {
        LOG( KVideoEditorLogFile, "CVeiSettingsContainer::HandleResourceChange() 1" );
        TRect rect;

        AknLayoutUtils::LayoutMetricsRect( AknLayoutUtils::EMainPane, rect );
        LOGFMT4( KVideoEditorLogFile, "CVeiSettingsContainer::HandleResourceChange(): 2, (%d,%d),(%d,%d)", rect.iTl.iX, rect.iTl.iY, rect.iBr.iX, rect.iBr.iY );
        SetRect( rect );
        }
    iSettingItemList->DrawNow();
    CCoeControl::HandleResourceChange( aType );

    LOG( KVideoEditorLogFile, "CVeiSettingsContainer::HandleResourceChange() Out" );
    }



void CVeiSettingsContainer::SizeChanged()
    {
    LOG( KVideoEditorLogFile, "CVeiSettingsContainer::SizeChanged(): In" );
    //TRect rect( Rect() ); 
    TRect rect;
    AknLayoutUtils::LayoutMetricsRect( AknLayoutUtils::EMainPane, rect );

    LOGFMT4( KVideoEditorLogFile, "CVeiSettingsContainer::SizeChanged(): 1, (%d,%d),(%d,%d)", rect.iTl.iX, rect.iTl.iY, rect.iBr.iX, rect.iBr.iY );

    LOG( KVideoEditorLogFile, "CVeiSettingsContainer::SizeChanged(): 2" );
    if ( iSettingItemList )
        {
        LOG( KVideoEditorLogFile, "CVeiSettingsContainer::SizeChanged(): 3" );
        iSettingItemList->SetRect( rect );
        }
    LOG( KVideoEditorLogFile, "CVeiSettingsContainer::SizeChanged(): Out" );
    }


// End of File
