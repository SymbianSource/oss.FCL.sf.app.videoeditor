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


#ifndef __VEIVIDEOEDITORSETTINS_H
#define __VEIVIDEOEDITORSETTINS_H

// INCLUDES
// System includes
#include    <e32base.h>

// CLASS DECLARATION

/**
*  Common settings handler
*
*  @lib VideoEditorCommon.lib
*  @since ?Series60_version
*/
NONSHARABLE_CLASS( CVeiVideoEditorSettings )
    {
    public: // New functions
        
        /**
        * GetMediaPlayerVolumeLevelL.
        * @since S60
        * @param aVolumeLevel returns volume level from MediaPlayer settings
        */
        IMPORT_C static void GetMediaPlayerVolumeLevelL( TInt& aVolumeLevel );
        
        /**
        * SetMediaPlayerVolumeLevelL.
        * @since ?Series60_version
        * @param aVolumeLevel save volume level. See <MPMediaPlayerSettings.h>
        *        for min / max values               
        */        
		IMPORT_C static void SetMediaPlayerVolumeLevelL( TInt aVolumeLevel );
        
        /**
        * GetMaxMmsSizeL.
        * @since ?Series60_version
        * @param aMaxMmsSize returns maximum MMS size
        */		
		IMPORT_C static void GetMaxMmsSizeL( TInt& aMaxMmsSize );
    };

#endif      // __VEIVIDEOEDITORSETTINS_H
            
// End of File
