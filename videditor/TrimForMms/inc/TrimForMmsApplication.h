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
 * File: TrimForMmsApplication.h
 * Created: 04/18/06
 * Author: 
 * 
 * ==================================================================== */

#ifndef __TRIMFORMMS_APPLICATION_H__
#define __TRIMFORMMS_APPLICATION_H__

#include <aknapp.h>


/*! 
  @class CTrimForMmsApplication
  
  @discussion An instance of CTrimForMmsApplication is the application part of the AVKON
  application framework for the TrimForMms example application
  */
class CTrimForMmsApplication : public CAknApplication
    {
public:  // from CAknApplication

/*! 
  @function AppDllUid
  
  @discussion Returns the application DLL UID value
  @result the UID of this Application/Dll
  */
    TUid AppDllUid() const;

protected: // from CAknApplication
/*! 
  @function CreateDocumentL
  
  @discussion Create a CApaDocument object and return a pointer to it
  @result a pointer to the created document
  */
    CApaDocument* CreateDocumentL();
    };

#endif // __TRIMFORMMS_APPLICATION_H__
