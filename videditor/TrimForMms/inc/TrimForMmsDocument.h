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
 * File: TrimForMmsDocument.h
 * Created: 04/18/06
 * Author: 
 * 
 * ==================================================================== */

#ifndef __TRIMFORMMS_DOCUMENT_H__
#define __TRIMFORMMS_DOCUMENT_H__


#include <akndoc.h>

// Forward references
class CTrimForMmsAppUi;
class CEikApplication;


/*! 
  @class CTrimForMmsDocument
  
  @discussion An instance of class CTrimForMmsDocument is the Document part of the AVKON
  application framework for the TrimForMms example application
  */
class CTrimForMmsDocument : public CAknDocument
    {
public:

/*!
  @function NewL
  
  @discussion Construct a CTrimForMmsDocument for the AVKON application aApp 
  using two phase construction, and return a pointer to the created object
  @param aApp application creating this document
  @result a pointer to the created instance of CTrimForMmsDocument
  */
    static CTrimForMmsDocument* NewL(CEikApplication& aApp);

/*!
  @function NewLC
  
  @discussion Construct a CTrimForMmsDocument for the AVKON application aApp 
  using two phase construction, and return a pointer to the created object
  @param aApp application creating this document
  @result a pointer to the created instance of CTrimForMmsDocument
  */
    static CTrimForMmsDocument* NewLC(CEikApplication& aApp);

/*!
  @function ~CTrimForMmsDocument
  
  @discussion Destroy the object and release all memory objects
  */
    ~CTrimForMmsDocument();

public: // from CAknDocument
/*!
  @function CreateAppUiL 
  
  @discussion Create a CTrimForMmsAppUi object and return a pointer to it
  @result a pointer to the created instance of the AppUi created
  */
    CEikAppUi* CreateAppUiL();


    void OpenFileL(CFileStore*& /*aFileStore*/, RFile& aFile);
    
private:

/*!
  @function ConstructL
  
  @discussion Perform the second phase construction of a CTrimForMmsDocument object
  */
    void ConstructL();

/*!
  @function CTrimForMmsDocument
  
  @discussion Perform the first phase of two phase construction 
  @param aApp application creating this document
  */
    CTrimForMmsDocument(CEikApplication& aApp);

    };


#endif // __TRIMFORMMS_DOCUMENT_H__
