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


/* ====================================================================
 * File: TrimForMmsAppUi.cpp
 * Created: 04/18/06
 * Author: 
 * 
 * ==================================================================== */

#include <avkon.hrh>
#include <aknnotewrappers.h> 
#include <trimformms.rsg>

#include "TrimForMms.pan"
#include "TrimForMmsAppUi.h"
#include "VeiSettings.h"
#include "VeiTrimForMmsView.h"
#include "TrimForMms.hrh"
#include "VideoEditorCommon.h"
#include "VideoEditorDebugUtils.h"

#include <sendui.h>

// ConstructL is called by the application framework
void CTrimForMmsAppUi::ConstructL()
    {

    BaseConstructL(EAppOrientationAutomatic|EAknEnableSkin);

    iSendUi = CSendUi::NewL();

    iTrimForMmsView = CVeiTrimForMmsView::NewL(*iSendUi);

    AddViewL (iTrimForMmsView);    // transfers ownership

    }

CTrimForMmsAppUi::CTrimForMmsAppUi()                              
    {
	// no implementation required
    }

CTrimForMmsAppUi::~CTrimForMmsAppUi()
    {
    if (iTrimForMmsView)
        {
        delete iTrimForMmsView;
        iTrimForMmsView = NULL;
        }
        
    delete iSendUi;
    }

// handle any menu commands
void CTrimForMmsAppUi::HandleCommandL(TInt aCommand)
    {
    switch(aCommand)
        {
        case EEikCmdExit:
        case EAknSoftkeyExit:
            Exit();
            break;

        default:
            Panic(ETrimForMmsBasicUi);
            break;
        }
    }

//=============================================================================
void CTrimForMmsAppUi::ReadSettingsL( TVeiSettings& aSettings ) 
	{
	LOG(KVideoEditorLogFile, "CTrimForMmsAppUi::ReadSettingsL: in");
	CDictionaryStore* store = Application()->OpenIniFileLC( iCoeEnv->FsSession() );

	TBool storePresent = store->IsPresentL( KUidVideoEditor );	// UID has an associated stream?

	if( storePresent ) 
		{
		RDictionaryReadStream readStream;
		readStream.OpenLC( *store, KUidVideoEditor );

		readStream >> aSettings;	// Internalize data to TVeiSettings.
		
		CleanupStack::PopAndDestroy( readStream ); 
		}
	else 
		{
		/* Read the default filenames from resources */
		HBufC*	videoName = iEikonEnv->AllocReadResourceLC( R_VEI_SETTINGS_VIEW_SETTINGS_ITEM_VALUE );

		const CFont* myFont = AknLayoutUtils::FontFromId( EAknLogicalFontSecondaryFont );

		aSettings.DefaultVideoName() = AknTextUtils::ChooseScalableText(videoName->Des(), *myFont, 400 );	
		CleanupStack::PopAndDestroy( videoName );

		HBufC*	snapshotName = iEikonEnv->AllocReadResourceLC( R_VEI_SETTINGS_VIEW_SETTINGS_ITEM2_VALUE );
		aSettings.DefaultSnapshotName() = AknTextUtils::ChooseScalableText(snapshotName->Des(), *myFont, 400 );
		CleanupStack::PopAndDestroy( snapshotName );

		/* Memory card is used as a default target */
		aSettings.MemoryInUse() = CAknMemorySelectionDialog::EMemoryCard;

         /* Set save quality to "Auto" by default. */
        aSettings.SaveQuality() = TVeiSettings::EAuto;

		RDictionaryWriteStream writeStream;
		writeStream.AssignLC( *store, KUidVideoEditor );

		writeStream << aSettings;

		writeStream.CommitL();

		store->CommitL();
		
		CleanupStack::PopAndDestroy( writeStream );	
		}
	CleanupStack::PopAndDestroy( store );
	LOG(KVideoEditorLogFile, "CTrimForMmsAppUi::ReadSettingsL: out");
	}

//=============================================================================
/*void CTrimForMmsAppUi::HandleScreenDeviceChangedL()
	{
	LOG(KVideoEditorLogFile, "CVeiAppUi::HandleScreenDeviceChangedL: In");
	CAknAppUi::HandleScreenDeviceChangedL(); 
	if ( iTrimForMmsView )
		{
		iTrimForMmsView->HandleScreenDeviceChangedL();
		}
	LOG(KVideoEditorLogFile, "CVeiAppUi::HandleScreenDeviceChangedL: Out");
	}*/

//=============================================================================
void CTrimForMmsAppUi::HandleResourceChangeL(TInt aType)
	{
	LOG(KVideoEditorLogFile, "CTrimForMmsAppUi::HandleResourceChangeL: In");
	CAknAppUi::HandleResourceChangeL(aType);
	if ( iTrimForMmsView )
		{
		iTrimForMmsView->HandleResourceChange(aType);
		}
	LOG(KVideoEditorLogFile, "CTrimForMmsAppUi::HandleResourceChangeL: Out");
	}

// End of File
