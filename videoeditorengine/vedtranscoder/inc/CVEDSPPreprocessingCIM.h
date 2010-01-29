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
* HwDevice plugin.
*
*/


/**
* @Example of the CI usage:
* 1. 
* MVEDSPPreprocessingCI* preprocCI;
* preprocCI = (MVEDSPPreprocessingCI*)iDevvrInstance->CustomInterface( hwdevUid, KVEDSPHwDevicePreprocCIUid );
* 
* where:
* CDEVVRVideoRecord iDevvrInstance - The inctance of the DeviceVideoRecord;
* TUid hwdevUid                    - Encode HwDevice Uid;
* KVEDSPHwDevicePreprocCIUid       - Custom Interface Uid;
* 
*
* 2. TInt errCode = preprocessCI->SetPreprocessing( preprocessMode );
* 
* where:
* TBool preprocessMode - Preprocessing mode setting; ETrue - ON / EFalse - OFF;
* 
* Preprocessing mode can be set ON / OFF only before initializing of the DeviceVideoRecord;
*
*/


#ifndef __MVEDSPPREPROCESSCI_H
#define __MVEDSPPREPROCESSCI_H


//  INCLUDES

// CONSTANTS
const TUid KVEDSPHwDevicePreprocCIUid =   {0x101F86D8};     // Preprocessing custom interface Uid


// MACROS

// DATA TYPES

// FORWARD DECLARATIONS

// CLASS DECLARATION

class MVEDSPPreprocessingCI
    {
    public:
        
        /**
        * Enables / Disables preprocessing mode
        * @since 2.6
        * @param aMode ETrue - enable preprocessing
        * @return KErrNone - Success, otherwise KErrNotReady - unable to process operation
        */
        virtual TInt SetPreprocessing( TBool aMode ) = 0;

    };





#endif
