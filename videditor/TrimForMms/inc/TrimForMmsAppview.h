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
 * File: TrimForMmsAppView.h
 * Created: 04/18/06
 * Author: 
 * 
 * ==================================================================== */

#ifndef __TRIMFORMMS_APPVIEW_H__
#define __TRIMFORMMS_APPVIEW_H__


#include <coecntrl.h>

// FORWARD DECLARATIONS
class CSendUi;

/*! 
  @class CTrimForMmsAppView
  
  @discussion An instance of the Application View object for the TrimForMms 
  example application
  */
class CTrimForMmsAppView : public CCoeControl
    {
public:

/*!
  @function NewL
   
  @discussion Create a CTrimForMmsAppView object, which will draw itself to aRect
  @param aRect the rectangle this view will be drawn to
  @result a pointer to the created instance of CTrimForMmsAppView
  */
    static CTrimForMmsAppView* NewL(const TRect& aRect);

/*!
  @function NewLC
   
  @discussion Create a CTrimForMmsAppView object, which will draw itself to aRect
  @param aRect the rectangle this view will be drawn to
  @result a pointer to the created instance of CTrimForMmsAppView
  */
    static CTrimForMmsAppView* NewLC(const TRect& aRect);


/*!
  @function ~CTrimForMmsAppView
  
  @discussion Destroy the object and release all memory objects
  */
     ~CTrimForMmsAppView();


public:  // from CCoeControl
/*!
  @function Draw
  
  @discussion Draw this CTrimForMmsAppView to the screen
  @param aRect the rectangle of this view that needs updating
  */
    void Draw(const TRect& aRect) const;
  

private:

/*!
  @function ConstructL
  
  @discussion  Perform the second phase construction of a CTrimForMmsAppView object
  @param aRect the rectangle this view will be drawn to
  */
    void ConstructL(const TRect& aRect);

/*!
  @function CTrimForMmsAppView
  
  @discussion Perform the first phase of two phase construction 
  */
    CTrimForMmsAppView();
    
    
    // Send UI
    CSendUi*        iSendUi;
    
    };


#endif // __TRIMFORMMS_APPVIEW_H__
