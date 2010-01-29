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
#include <manualvideoeditor.rsg>
#include <sendui.h>     // CSendAppUi 
#include <stringloader.h>
#include <bautils.h>
#include <apparc.h>

// User includes
#include "manualvideoeditor.hrh"
#include "VeiAppUi.h"
#include "VeiEditVideoView.h"
#include "VeiSettingsView.h"
#include "VeiCutVideoView.h"
#include "VeiCutAudioView.h"
#include "VeiTrimForMmsView.h"
#include "VeiTempMaker.h"
#include "VideoEditorCommon.h"


// ================= MEMBER FUNCTIONS =======================
//
// ----------------------------------------------------------
// CVeiAppUi::ConstructL()
// ?implementation_description
// ----------------------------------------------------------
//
void CVeiAppUi::ConstructL()
    {
    LOG( KVideoEditorLogFile, "CVeiAppUi::ConstructL In" );

   	BaseConstructL( EAppOrientationAutomatic | EAknEnableSkin );

    CVeiTempMaker* maker = CVeiTempMaker::NewL();
    maker->EmptyTempFolder();
    delete maker;

    iSendAppUi = CSendUi::NewL();
    /*
     *	Cut video view and Cut audio view are references to edit video view and
     *	ownerships must be transfered(AddViewL(...)) to CAknViewAppUi(this) 
     *	AFTER references are taken to edit video view. Otherwise exit is not clean.
     */

    // Cut Video view
    iCutVideoView = new( ELeave )CVeiCutVideoView;
    iCutVideoView->ConstructL();

    // Cut Audio view
    iCutAudioView = CVeiCutAudioView::NewL();

    // Edit Video view
    iEditVideoView = CVeiEditVideoView::NewL( *iCutVideoView, * iCutAudioView, * iSendAppUi );
        
    AddViewL( iEditVideoView ); // transfer ownership to CAknViewAppUi
    LOG( KVideoEditorLogFile, "CVeiAppUi::ConstructL EditVideoView OK" );

    AddViewL( iCutAudioView );
    LOG( KVideoEditorLogFile, "CVeiAppUi::ConstructL CutAudioView OK" );

    AddViewL( iCutVideoView ); // transfer ownership to CAknViewAppUi
    LOG( KVideoEditorLogFile, "CVeiAppUi::ConstructL CutVideoView OK" );

    //Trim for MMS view
    iTrimForMmsView = CVeiTrimForMmsView::NewL( *iSendAppUi );
    AddViewL( iTrimForMmsView ); // Transfer ownership to CAknViewAppUi
    LOG( KVideoEditorLogFile, "CVeiAppUi::ConstructL TrimForMmsView OK" );

    //Settings view
    iSettingsView = CVeiSettingsView::NewL();
    AddViewL( iSettingsView ); // Transfer ownership to CAknViewAppUi
    LOG( KVideoEditorLogFile, "CVeiAppUi::ConstructL SettingsView OK" );

    iVolume =  - 1; // Volume not set

    iCoeEnv->RootWin().EnableScreenChangeEvents();

    SetDefaultViewL( *iEditVideoView );

    LOG( KVideoEditorLogFile, "CVeiAppUi::ConstructL Out" );
    }

// ----------------------------------------------------
// CVeiAppUi::~CVeiAppUi()
// Destructor
// Frees reserved resources
// ----------------------------------------------------
//
CVeiAppUi::~CVeiAppUi()
    {
    LOG( KVideoEditorLogFile, "CVeiAppUi::~CVeiAppUi: In" );
   
    delete iSendAppUi;

    LOG( KVideoEditorLogFile, "CVeiAppUi::~CVeiAppUi: Out" );
    }

//=============================================================================
CVeiAppUi::CVeiAppUi()
    {
    }

//=============================================================================
void CVeiAppUi::InsertVideoClipToMovieL( TBool aDoOpen, const TDesC& aFilename )
    {
    LOGFMT( KVideoEditorLogFile, "CVeiAppUi::InsertVideoClipTomovieL (%S)", &aFilename );
           

    if ( iEditVideoView )
        {
        iEditVideoView->AddClipL( aFilename, aDoOpen );
        }
    }

// ----------------------------------------------------
// CVeiAppUi::HandleKeyEventL(
//     const TKeyEvent& aKeyEvent,TEventCode /*aType*/)
// ?implementation_description
// ----------------------------------------------------
//
TKeyResponse CVeiAppUi::HandleKeyEventL( const TKeyEvent&  /*aKeyEvent*/,
                                         TEventCode /*aType*/ )
    {
    return EKeyWasNotConsumed;
    }


// ----------------------------------------------------
// CVeiAppUi::HandleCommandL(TInt aCommand)
// ?implementation_description
// ----------------------------------------------------
//
void CVeiAppUi::HandleCommandL( TInt aCommand )
    {
    LOGFMT( KVideoEditorLogFile, "CVeiAppUi::HandleCommandL( %d ): In", aCommand );
           
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
        case EVeiCmdSettingsViewHelp:
        case EVeiCmdTrimForMmsViewHelp:
        case EVeiCmdEditVideoViewHelp:
        case EVeiCmdCutVideoViewHelp:
                {
                // Get the current context
                CArrayFix < TCoeHelpContext > * context = AppHelpContextL();

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

                iEditVideoView->HandleCommandL( EAknSoftkeyOk );
                Exit();
                break;
                }
        default:
            break;
        }
    LOG( KVideoEditorLogFile, "CVeiAppUi::HandleCommandL: Out" );
    }

//=============================================================================
void CVeiAppUi::ReadSettingsL( TVeiSettings& aSettings )const
    {
    LOG( KVideoEditorLogFile, "CVeiAppUi::ReadSettingsL: in" );
    CDictionaryStore* store = Application()->OpenIniFileLC( iCoeEnv->FsSession() );
        

    TBool storePresent = store->IsPresentL( KUidVideoEditor );  // UID has an associated stream?

    if ( storePresent )
        {
        RDictionaryReadStream readStream;
        readStream.OpenLC( *store, KUidVideoEditor );

        readStream >> aSettings; // Internalize data to TVeiSettings.

        CleanupStack::PopAndDestroy( &readStream );
        }
    else
        {
        /* Read the default filenames from resources */
        HBufC* videoName = iEikonEnv->AllocReadResourceLC( R_VEI_SETTINGS_VIEW_SETTINGS_ITEM_VALUE );

        const CFont* myFont = AknLayoutUtils::FontFromId( EAknLogicalFontSecondaryFont );

        aSettings.DefaultVideoName() = AknTextUtils::ChooseScalableText( videoName->Des(), * myFont, 400 );
        CleanupStack::PopAndDestroy( videoName );

        HBufC* snapshotName = iEikonEnv->AllocReadResourceLC( R_VEI_SETTINGS_VIEW_SETTINGS_ITEM2_VALUE );
            
        aSettings.DefaultSnapshotName() = AknTextUtils::ChooseScalableText( snapshotName->Des(), * myFont, 400 );
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
    LOG( KVideoEditorLogFile, "CVeiAppUi::ReadSettingsL: out" );
    }

//=============================================================================
void CVeiAppUi::WriteSettingsL( const TVeiSettings& aSettings )const
    {
    LOG( KVideoEditorLogFile, "CVeiAppUi::WriteSettingsL: in" );
    CDictionaryStore* store = Application()->OpenIniFileLC( iCoeEnv->FsSession() );

    RDictionaryWriteStream writeStream;
    writeStream.AssignLC( *store, KUidVideoEditor );
    writeStream << aSettings;
    writeStream.CommitL();

    store->CommitL();

    CleanupStack::PopAndDestroy( &writeStream );
    CleanupStack::PopAndDestroy( &store );    
    LOG( KVideoEditorLogFile, "CVeiAppUi::WriteSettingsL: out" );
    }

//=============================================================================
void CVeiAppUi::HandleScreenDeviceChangedL()
    {
    LOG( KVideoEditorLogFile, "CVeiAppUi::HandleScreenDeviceChangedL: In" );
    CAknViewAppUi::HandleScreenDeviceChangedL();
    if ( iEditVideoView )
        {
        iEditVideoView->HandleScreenDeviceChangedL();
        }
    LOG( KVideoEditorLogFile, "CVeiAppUi::HandleScreenDeviceChangedL: Out" );
    }

//=============================================================================
void CVeiAppUi::HandleResourceChangeL( TInt aType )
    {
    LOG( KVideoEditorLogFile, "CVeiAppUi::HandleResourceChangeL: In" );
    CAknAppUi::HandleResourceChangeL( aType );
    if ( iEditVideoView )
        {
        iEditVideoView->HandleResourceChange( aType );
        }
    if ( iCutVideoView )
        {
        iCutVideoView->HandleResourceChange( aType );
        }
    if ( iCutAudioView )
        {
        iCutAudioView->HandleResourceChange( aType );
        }
    if ( iTrimForMmsView )
        {
        iTrimForMmsView->HandleResourceChange( aType );
        }
    LOG( KVideoEditorLogFile, "CVeiAppUi::HandleResourceChangeL: Out" );
    }

//=============================================================================
void CVeiAppUi::HandleFileNotificationEventL()
    {
    LOG( KVideoEditorLogFile, "CVeiAppUi::HandleFileNotificationEventL: In" );
    if ( iEditVideoView )
        {
        if ( iEditVideoView->WaitMode() == CVeiEditVideoView ::EProcessingMovieSaveThenQuit )
            {
            HandleCommandL( EAknCmdExit );
            }
        }
    LOG( KVideoEditorLogFile, "CVeiAppUi::HandleFileNotificationEventL: Out" );
    }

//=============================================================================
void CVeiAppUi::HandleForegroundEventL( TBool aForeground )
    {
    LOGFMT( KVideoEditorLogFile, "CVeiAppUi::HandleForegroundEventL: In: %d", aForeground );
    CAknViewAppUi::HandleForegroundEventL( aForeground );
    if ( !aForeground )
        {
        // Set the priority to low. This is needed to handle the situations 
        // where the engine is performing heavy processing while the application 
        // is in background.
        RProcess myProcess;
        iOriginalProcessPriority = myProcess.Priority();
        LOGFMT3( KVideoEditorLogFile, 
                "CVeiAppUi::HandleForegroundEventL: changing priority of process %Ld from %d to %d", myProcess.Id().Id(), iOriginalProcessPriority, EPriorityLow );
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
        LOGFMT2( KVideoEditorLogFile, 
                "CVeiAppUi::HandleForegroundEventL: process %Ld back to normal priority %d", myProcess.Id().Id(), priority );
        }

    LOG( KVideoEditorLogFile, "CVeiAppUi::HandleForegroundEventL: Out" );
    }

//=============================================================================
TErrorHandlerResponse CVeiAppUi::HandleError ( TInt aError,
                                               const SExtendedError & aExtErr,
                                               TDes & aErrorText,
                                               TDes & aContextText )
    {
    LOGFMT( KVideoEditorLogFile, "CVeiAppUi::HandleError: %d", aError );

    // Let the framework handle errors
	return CAknViewAppUi::HandleError ( aError, aExtErr, aErrorText, aContextText );
    }

// End of File
