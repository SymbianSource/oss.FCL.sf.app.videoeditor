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
//DisplayChain.cpp
//
//Version 1.00
//
//Contains:
//	CDisplayChain 
//		Image processing class implementing
//		display specific image processing:
//			- Scaling to display size with pan support
//			- IETD - display specific color contrast enhancement
//			- Sharpening 
//			- Display feature compensation by simple color management
//			- Dithering 
//			
//History:
//	30.10.2003 version 1.00 created using existing IMAAMI algorithms	
//*************************************************************************

#ifndef __DisplayChain_H
#define __DisplayChain_H

#include <e32std.h> // for Fundamental Types
#include <e32base.h> // for Fundamental Types
#include <e32math.h> //for the calculation of exponents
#include <stdio.h>

#include "DCDigitalZoom.h"
#include "DCSharpening.h"
#include "DCIetd.h"
#include "DCDithering.h"
#include "DCColorManagement.h"
#include "DCDefines.h"
#include "DCInit.h"


//Class definition
class CDisplayChain : public CBase
{
public:

	//Standard functions
	CDisplayChain();
	static CDisplayChain* NewL();
	static CDisplayChain* NewLC();
	~CDisplayChain();
	
	// Second Phase contructor (may leave)
	void ConstructL();
	
	//Process original image and store result
	void ProcessL(const CFbsBitmap* InBitmap, CFbsBitmap* aTargetBitmap);

	//Processing parameters
	struct DisplayChainParams
	{
		TBool FirstOpening;

		TBool IetdON;				//IETD ON/OFF
		TBool SharpeningON;			//Sharpening ON/OFF
		TBool ColorManagementON;	//Color Management ON/OFF
		TBool DitheringON;			//Dithering ON/OFF
		TBool DigitalZoomON;		//Scaling ON/OFF

		//Parameters for each processing algorithm
		//See processing algorithm classes for details.
		CDCIetd::DCIetdParameters IETDParameters;
		DCSharpening::DCSharpeningParams SharpeningParameters;
		CDCColorManagement::DCColorManagementParams ColorManagementParameters;
		CDCDigitalZoom::DCDigitalZoomParams DZParameters;
	};	
	
	//Parameter excange
	void SetParameters(DisplayChainParams* Parameters);
	void GetParameters(DisplayChainParams* Parameters);
	
	//Limit integer value to byte [0,255]
	static inline TUint8 Limit255(TInt i) {return (TUint8)(i<0?0:(i>255?255:i));}
	
private:


	//PArameters
	DisplayChainParams Parameters;

	//Processing objects
	CDCIetd* IetdPtr;
	DCSharpening* SharpeningPtr;
	CDCColorManagement* ColorManagementPtr;
	CDCDithering* DitheringPtr;
	CDCDigitalZoom* DigitalZoomPtr;
};

#endif // ifndef __DisplayChain_H
//----IMAAMI----
