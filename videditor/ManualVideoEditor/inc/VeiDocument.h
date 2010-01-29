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


#ifndef VEDDOCUMENT_H
#define VEDDOCUMENT_H

// INCLUDES
#include <genericparamconsumer.h>

// CONSTANTS

// FORWARD DECLARATIONS
class CEikAppUi;

// CLASS DECLARATION

/**
 *  CVeiDocument application class.
 */
class CVeiDocument: public CAiwGenericParamConsumer
{
public:
    // Constructors and destructor
    /**
     * Two-phased constructor.
     */
    static CVeiDocument* NewL( CEikApplication& aApp );

    //CFileStore* OpenFileL(TBool aDoOpen,const TDesC& aFilename,RFs& aFs);
    //void OpenFileL(TBool aDoOpen , RFile& aFile);
    void OpenFileL( CFileStore* & aFileStore, RFile& aFile );

    /**
     * Destructor.
     */
    virtual ~CVeiDocument();
    //		virtual void RestoreL(const CStreamStore& aStore, const CStreamDictionary& aStreamDic);

private:

    /**
     * Default constructor.
     */
    CVeiDocument( CEikApplication& aApp );
    void ConstructL();

private:

    /**
     * From CEikDocument, create CVeiAppUi "App UI" object.
     */
    CEikAppUi* CreateAppUiL();
};

#endif 

// End of File
