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
* Common settings.
*
*/



// INCLUDES
// System includes
#include <centralrepository.h>
#include <MmsEngineDomainCRKeys.h>
#include <MPMediaPlayerSettings.h>
#include <e32property.h>

// User includes
#include "VeiVideoEditorSettings.h"
#include "VideoEditorCommon.h"
#include "VideoEditorDebugUtils.h"


// CONSTANTS

EXPORT_C void CVeiVideoEditorSettings::GetMediaPlayerVolumeLevelL( TInt& aVolumeLevel )
	{
    LOG(KVideoEditorLogFile, "CVeiVideoEditorSettings::GetMediaPlayerVolumeLevelL(): In");
    CMPMediaPlayerSettings* mpSettings = CMPMediaPlayerSettings::NewL();
    CleanupStack::PushL( mpSettings );
    
    aVolumeLevel = mpSettings->VolumeLevelL();
    
    CleanupStack::PopAndDestroy( mpSettings );
    LOG(KVideoEditorLogFile, "CVeiVideoEditorSettings::GetMediaPlayerVolumeLevelL(): Out");
	}

EXPORT_C void CVeiVideoEditorSettings::SetMediaPlayerVolumeLevelL( TInt aVolumeLevel )
	{
    LOG(KVideoEditorLogFile, "CVeiVideoEditorSettings::SetMediaPlayerVolumeLevelL(): In");
    
    CMPMediaPlayerSettings* mpSettings = CMPMediaPlayerSettings::NewL();
    CleanupStack::PushL( mpSettings );
    
    mpSettings->SetVolumeLevelL( aVolumeLevel );
    
    CleanupStack::PopAndDestroy( mpSettings );    
    LOG(KVideoEditorLogFile, "CVeiVideoEditorSettings::SetMediaPlayerVolumeLevelL(): Out");
	}

EXPORT_C void CVeiVideoEditorSettings::GetMaxMmsSizeL( TInt& aMaxMmsSize )
	{
    LOG(KVideoEditorLogFile, "CVeiVideoEditorSettings::GetMaxMmsSizeL(): In");

	CRepository* repository = CRepository::NewL(KCRUidMmsEngine);
	repository->Get(KMmsEngineMaximumSendSize, aMaxMmsSize);
	LOGFMT(KVideoEditorLogFile, "CVeiVideoEditorSettings::GetMaxMmsSizeL() 1, KMmsEngineMaximumSendSize:%d", aMaxMmsSize);
	delete repository;

    LOG(KVideoEditorLogFile, "CVeiVideoEditorSettings::GetMaxMmsSizeL(): Out");
	}
	
// End of File
