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
 * File: TrimForMmsAppUi.h
 * Created: 04/18/06
 * Author: 
 * 
 * ==================================================================== */

#ifndef __TRIMFORMMS_APPUI_H__
#define __TRIMFORMMS_APPUI_H__

#include <aknviewappui.h>

// Forward reference
class CVeiTrimForMmsView;
class CSendUi;
class TVeiSettings;

/*! 
  @class CTrimForMmsAppUi
  
  @discussion An instance of class CTrimForMmsAppUi is the UserInterface part of the AVKON
  application framework for the TrimForMms example application
  */
class CTrimForMmsAppUi : public CAknViewAppUi
    {
public:
/*!
  @function ConstructL
  
  @discussion Perform the second phase construction of a CTrimForMmsAppUi object
  this needs to be public due to the way the framework constructs the AppUi 
  */
    void ConstructL();

/*!
  @function CTrimForMmsAppUi
  
  @discussion Perform the first phase of two phase construction.
  This needs to be public due to the way the framework constructs the AppUi 
  */
    CTrimForMmsAppUi();


/*!
  @function ~CTrimForMmsAppUi
  
  @discussion Destroy the object and release all memory objects
  */
    ~CTrimForMmsAppUi();


/*!
  Reads application settings data from ini-file. 
 
 @param aSettings Settings data where values are read.
 */
	void ReadSettingsL( TVeiSettings& aSettings );


public: // from CAknAppUi
/*!
  @function HandleCommandL
  
  @discussion Handle user menu selections
  @param aCommand the enumerated code for the option selected
  */
    void HandleCommandL(TInt aCommand);

/*!
  Calls CAknAppUiBase::HandleScreenDeviceChangedL().
  */
//	virtual void HandleScreenDeviceChangedL();	

	/**
    * From @c CEikAppUi. Handles a change to the application's resources which
    * are shared across the environment. This function calls 
    * @param aType The type of resources that have changed. 
    */
	virtual void HandleResourceChangeL(TInt aType);

private:
/*! @var iTrimForMmsView The application view */
    CVeiTrimForMmsView*     iTrimForMmsView;
    
    CSendUi*                iSendUi;
    };


#endif // __TRIMFORMMS_APPUI_H__

