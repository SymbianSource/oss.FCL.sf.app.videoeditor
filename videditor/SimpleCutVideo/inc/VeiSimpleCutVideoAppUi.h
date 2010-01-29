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



#ifndef VEDSIMPLECUTVIDEOAPPUI_H
#define VEDSIMPLECUTVIDEOAPPUI_H

// INCLUDES
// System includes
#include <eikapp.h>
#include <eikdoc.h>
#include <e32std.h>
#include <coeccntx.h>
#include <aknviewappui.h>
#include <akntabgrp.h>
#include <aknnavide.h>
#include "VeiSettings.h"
#include "VideoEditorDebugUtils.h"

// FORWARD DECLARATIONS
class CVeiSimpleCutVideoView;
class CEikAppUi;


/**
* Application UI class.
* Provides support for the following features:
* - EIKON control architecture
* - view architecture
* - status pane
* 
*/
class CVeiSimpleCutVideoAppUi : public CAknViewAppUi

    {
    public: // // Constructors and destructor

        /**
        * Default constructor.
        */      
        void ConstructL();

        /**
        * Destructor.
        */      
        ~CVeiSimpleCutVideoAppUi();

    public: // New functions
		inline TInt GetVolumeLevel() { return iVolume; };
		
		inline void SetVolumeLevel( TInt aVolume ) { iVolume=aVolume; };

		void CutVideoL( TBool aDoOpen, const RFile& aFile );

		CVeiSimpleCutVideoAppUi();
		
		void Exit();

		/**
		 * Reads application settings data from ini-file. 
		 *
		 * @param aSettings Settings data where values are read.
		 */		
		void ReadSettingsL( TVeiSettings& aSettings ) const;
		
		/**
		 * Writes application settings data to ini-file.
		 *
		 * @param aSettings Settings data where values are written.
		 */
		void WriteSettingsL( const TVeiSettings& aSettings );
		
		inline TBool AppIsOnTheWayToDestruction() { return iOnTheWayToDestruction; };

    private:
        /**
        * From CEikAppUi, takes care of command handling.
        * @param aCommand command to be handled
        */
        void HandleCommandL(TInt aCommand);

        /**
        * From CEikAppUi, handles key events.
        * @param aKeyEvent Event to handled.
        * @param aType Type of the key event. 
        * @return Response code (EKeyWasConsumed, EKeyWasNotConsumed). 
        */
        virtual TKeyResponse HandleKeyEventL(
            const TKeyEvent& aKeyEvent,TEventCode aType);

        /**
        * From CAknAppUiBase.   
	    * Calls CAknViewAppUi::HandleScreenDeviceChangedL().
        */
		virtual void HandleScreenDeviceChangedL();	

		/**
	    * From @c CEikAppUi. Handles a change to the application's resources which
	    * are shared across the environment. This function calls 
	    * @param aType The type of resources that have changed. 
	    */
		virtual void HandleResourceChangeL(TInt aType);

		/** 
		* From CAknAppUi, HandleForegroundEventL( TBool aForeground )
		* @param aForeground
		*/
		virtual void HandleForegroundEventL( TBool aForeground );
		
		// From MMGXFileNotificationObserver
		virtual void HandleFileNotificationEventL();

    private: //Data
    
    	CVeiSimpleCutVideoView*	iSimpleCutVideoView;
        
        /**
         * Common volume setting.
         */
		TInt				iVolume;

		TProcessPriority 	iOriginalProcessPriority;
		TBool 				iProcessPriorityAltered;
		
		TBool 				iOnTheWayToDestruction;
	};

#endif

// End of File
