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
// System includes
#include <avkon.hrh>
#include <eikmenup.h>
#include <eikenv.h>
#include <hlplch.h>     // HlpLauncher
#include <VedSimpleCutVideo.rsg>
#include <stringloader.h>
#include <bautils.h>
#include <mgfetch.h> 
#include <data_caging_path_literals.hrh>

// User includes
#include "VedSimpleCutVideo.hrh"
#include "veisimplecutvideoappui.h"
#include "veisimplecutvideoview.h"

#include "VeiTempMaker.h"
#include "VideoEditorCommon.h"

// ================= MEMBER FUNCTIONS =======================
//
// ----------------------------------------------------------
// CVeiSimpleCutVideoAppUi::ConstructL()
// ?implementation_description
// ----------------------------------------------------------
//
void CVeiSimpleCutVideoAppUi::ConstructL()
    {
    LOG(KVideoEditorLogFile, "CVeiSimpleCutVideoAppUi::ConstructL: In");

   	BaseConstructL( EAppOrientationAutomatic|EAknEnableSkin|EAknEnableMSK );
	
	CVeiTempMaker* maker = CVeiTempMaker::NewL();
	maker->EmptyTempFolder();
	delete maker;
	
/*
*	Cut video view and Cut audio view are references to edit video view and
*	ownerships must be transfered(AddViewL(...)) to CAknViewAppUi(this) 
*	AFTER references are taken to edit video view. Otherwise exit is not clean.
*/


// Cut Video view	    
	/*iSimpleMergeView = CVeiSimpleCutVideoView::NewLC( ClientRect() );
	AddViewL( iSimpleMergeView );  // transfer ownership to CAknViewAppUi
	CleanupStack::Pop( iSimpleMergeView );
	*/

    LOG(KVideoEditorLogFile, "CVeiSimpleCutVideoAppUi::ConstructL: Out");
    }

// ----------------------------------------------------
// CVeiSimpleCutVideoAppUi::~CVeiSimpleCutVideoAppUi()
// Destructor
// Frees reserved resources
// ----------------------------------------------------
//
CVeiSimpleCutVideoAppUi::~CVeiSimpleCutVideoAppUi()
    {
    LOG(KVideoEditorLogFile, "CVeiSimpleCutVideoAppUi::~CVeiSimpleCutVideoAppUi()");
	}
	
void CVeiSimpleCutVideoAppUi::Exit()
	{
	LOG(KVideoEditorLogFile, "CVeiSimpleCutVideoAppUi::Exit(): In");

	TRAP_IGNORE ( 
		CVeiTempMaker* maker = CVeiTempMaker::NewL();
		maker->EmptyTempFolder();
		delete maker; 
		);

	CAknAppUiBase::Exit();
	
	LOG(KVideoEditorLogFile, "CVeiSimpleCutVideoAppUi::Exit(): Out");
	}

//=============================================================================
CVeiSimpleCutVideoAppUi::CVeiSimpleCutVideoAppUi()
	{
	}

//=============================================================================
void CVeiSimpleCutVideoAppUi::CutVideoL( TBool aDoOpen, const RFile& aFile )
	{
    LOG(KVideoEditorLogFile, "CVeiSimpleCutVideoAppUi::CutVideoL: In");

	// Cut Video view
	if ( !iSimpleCutVideoView )
		{		
		CVeiSimpleCutVideoView* simpleCutVideoView = 
		    new (ELeave) CVeiSimpleCutVideoView;
		CleanupStack::PushL( simpleCutVideoView );
		simpleCutVideoView->ConstructL();
		iSimpleCutVideoView = simpleCutVideoView;
	    TFileName filename;
	    User::LeaveIfError( aFile.FullName(filename) );
	    simpleCutVideoView->AddClipL( filename, aDoOpen );
		
		AddViewL(simpleCutVideoView);
		CleanupStack::Pop( simpleCutVideoView );
		iVolume = -1;		// Volume not set

	   	iCoeEnv->RootWin().EnableScreenChangeEvents(); 

		SetDefaultViewL( *simpleCutVideoView );   
		ActivateLocalViewL(simpleCutVideoView->Id());		
		}

	LOG(KVideoEditorLogFile, "CVeiSimpleCutVideoAppUi::CutVideoL: Out");
	}

// ----------------------------------------------------
// CVeiSimpleCutVideoAppUi::HandleKeyEventL(
//     const TKeyEvent& aKeyEvent,TEventCode /*aType*/)
// ?implementation_description
// ----------------------------------------------------
//
TKeyResponse CVeiSimpleCutVideoAppUi::HandleKeyEventL(
    const TKeyEvent& /*aKeyEvent*/,TEventCode /*aType*/)
    {
    return EKeyWasNotConsumed;
    }


// ----------------------------------------------------
// CVeiSimpleCutVideoAppUi::HandleCommandL(TInt aCommand)
// ?implementation_description
// ----------------------------------------------------
//
void CVeiSimpleCutVideoAppUi::HandleCommandL( TInt aCommand )
    {
    LOGFMT(KVideoEditorLogFile, "CVeiSimpleCutVideoAppUi::HandleCommandL( %d ): In", aCommand);
    
    switch ( aCommand )
        {
        //
        // Context Sensitive Help launching for:
        //  -'Settings' view
        //  -'Trim for MMS' view
        //  -'Edit video' view
        //  -'Cut video' view
        //  -'Cut audio' view
        //
    case EAknCmdHelp:        
    case EVeiCmdCutVideoViewHelp:
            {
            // Get the current context
            CArrayFix<TCoeHelpContext>* context = AppHelpContextL();

            // Launch the help application with current context topic
            HlpLauncher::LaunchHelpApplicationL( iEikonEnv->WsSession(),
                                                 context );
            break;
            }
    case EAknSoftkeyBack:
    case EEikCmdExit:
    case EAknSoftkeyExit:
    case EAknCmdExit:
            {
            iOnTheWayToDestruction = ETrue;
            
            CVeiTempMaker* maker = CVeiTempMaker::NewL();
            maker->EmptyTempFolder();
            delete maker;
				
            Exit();
            break;
            }
        default:
            break;      
        }
        LOG(KVideoEditorLogFile, "CVeiAppUi::HandleCommandL: Out");
    }

//=============================================================================
void CVeiSimpleCutVideoAppUi::ReadSettingsL( TVeiSettings& aSettings ) const
	{
	LOG(KVideoEditorLogFile, "CVeiAppUi::ReadSettingsL: in");
	CDictionaryStore* store = Application()->OpenIniFileLC( iCoeEnv->FsSession() );

	TBool storePresent = store->IsPresentL( KUidVideoEditor );	// UID has an associated stream?

	if( storePresent ) 
		{
		RDictionaryReadStream readStream;
		readStream.OpenLC( *store, KUidVideoEditor );

		readStream >> aSettings;	// Internalize data to TVeiSettings.
		
		CleanupStack::PopAndDestroy( &readStream );
		}
	else {
		// In the case of simple cut, the video name is generated automatically
		// when saving is started.
		aSettings.DefaultVideoName() = KNullDesC;

		/* Read the default snapshot filename from resource */
		const CFont* myFont = AknLayoutUtils::FontFromId( EAknLogicalFontSecondaryFont );
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
		
		CleanupStack::PopAndDestroy( &writeStream );	
		}
	CleanupStack::PopAndDestroy( store );
	LOG(KVideoEditorLogFile, "CVeiAppUi::ReadSettingsL: out");
	}

//=============================================================================
void CVeiSimpleCutVideoAppUi::WriteSettingsL( const TVeiSettings& aSettings ) 
	{
	LOG(KVideoEditorLogFile, "CVeiAppUi::WriteSettingsL: in");
	CDictionaryStore* store = Application()->OpenIniFileLC( iCoeEnv->FsSession() );

	RDictionaryWriteStream writeStream;
	writeStream.AssignLC( *store, KUidVideoEditor );
	writeStream << aSettings;
	writeStream.CommitL();

	store->CommitL();

	CleanupStack::PopAndDestroy( &writeStream );
	CleanupStack::PopAndDestroy( store );
	LOG(KVideoEditorLogFile, "CVeiAppUi::WriteSettingsL: out");
	}

//=============================================================================
void CVeiSimpleCutVideoAppUi::HandleScreenDeviceChangedL()
	{
	LOG(KVideoEditorLogFile, "CVeiSimpleCutVideoAppUi::HandleScreenDeviceChangedL: In");

	CAknAppUi::HandleScreenDeviceChangedL();

	LOG(KVideoEditorLogFile, "CVeiSimpleCutVideoAppUi::HandleScreenDeviceChangedL: Out");
	}

//=============================================================================
void CVeiSimpleCutVideoAppUi::HandleResourceChangeL(TInt aType)
	{
	LOG(KVideoEditorLogFile, "CVeiSimpleCutVideoAppUi::HandleResourceChangeL: In");
	CAknAppUi::HandleResourceChangeL(aType);
	if ( iSimpleCutVideoView )
		{
		iSimpleCutVideoView->HandleResourceChange(aType);
		}
	LOG(KVideoEditorLogFile, "CVeiSimpleCutVideoAppUi::HandleResourceChangeL: Out");
	}

//=============================================================================
void CVeiSimpleCutVideoAppUi::HandleFileNotificationEventL()
	{
	LOG(KVideoEditorLogFile, "CVeiSimpleCutVideoAppUi::HandleFileNotificationEventL: In");
	/*if ( iSimpleCutVideoView )
		{
		if( iSimpleCutVideoView->WaitMode() == CVeiEditVideoView::EProcessingMovieSaveThenQuit )
			{
			HandleCommandL( EAknCmdExit );
			}
		}*/
	LOG(KVideoEditorLogFile, "CVeiSimpleCutVideoAppUi::HandleFileNotificationEventL: Out");
	}

//=============================================================================
void CVeiSimpleCutVideoAppUi::HandleForegroundEventL  ( TBool aForeground )
	{
	LOGFMT(KVideoEditorLogFile, "CVeiSimpleCutVideoAppUi::HandleForegroundEventL: In: %d", aForeground);
	CAknViewAppUi::HandleForegroundEventL( aForeground );
	if ( !aForeground )
		{
		// Set the priority to low. This is needed to handle the situations 
		// where the engine is performing heavy processing while the application 
		// is in background.
		RProcess myProcess;
		iOriginalProcessPriority = myProcess.Priority();
		LOGFMT3(KVideoEditorLogFile, "CVeiSimpleCutVideoAppUi::HandleForegroundEventL: changing priority of process %Ld from %d to %d", myProcess.Id().Id(), iOriginalProcessPriority, EPriorityLow);
		myProcess.SetPriority( EPriorityLow );
		iProcessPriorityAltered = ETrue;
		}
	else if ( iProcessPriorityAltered )
		{
		// Return to normal priority.
		RProcess myProcess;
		TProcessPriority priority = myProcess.Priority();
		if ( priority < iOriginalProcessPriority )
			{
			myProcess.SetPriority( iOriginalProcessPriority );
			}
		iProcessPriorityAltered = EFalse;
		LOGFMT2(KVideoEditorLogFile, "CVeiSimpleCutVideoAppUi::HandleForegroundEventL: process %Ld back to normal priority %d", myProcess.Id().Id(), priority);
		}

	LOG(KVideoEditorLogFile, "CVeiSimpleCutVideoAppUi::HandleForegroundEventL: Out");
	}

// End of File
