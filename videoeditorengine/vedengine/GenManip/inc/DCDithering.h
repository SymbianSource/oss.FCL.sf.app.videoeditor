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


//----IMAAMI----
//*************************************************************************
//DCDithering.h
//
//Version 1.00
//
//Contains:
//	CDCDithering 
//		Dithering by H263 algorithm developed in /.
//			
//History:
//	19.08.2003 version 1.00 created using existing algorithms	
//*************************************************************************



/*
-----------------------------------------------------------------------------

    DESCRIPTION

    Defines the dithering class.

-----------------------------------------------------------------------------
*/

#ifndef  __DCDithering_H__
#define  __DCDithering_H__



//  INCLUDES
#ifndef   __E32STD_H__
#include  <e32std.h>   // for Fundamental Types
#endif // __E32STD_H__
#ifndef   __E32BASE_H__
#include  <e32base.h>  // for CBase
#endif // __E32BASE_H__



//Class definition
class CDCDithering : public CBase
{
public:

	CDCDithering();					// Constructor
	static CDCDithering* NewL();	// Factory function
	static CDCDithering* NewLC();	// Factory function
	~CDCDithering();				// Destructor
	void ConstructL();				// Second Phase contructor (may leave)
	
	// Process and store image referenced by aBPtr
	void ProcessL(CFbsBitmap& aBPtr);
	
private:

	// Limit integer value to byte [0,255]
	static inline TUint8 Limit255(TInt i) {return (TUint8)(i<0?0:(i>255?255:i));}

	// Scan line buffer
	HBufC8*  iScanLineBuffer;
};

#endif // __DCDithering_H__
// End of File
//----IMAAMI----
