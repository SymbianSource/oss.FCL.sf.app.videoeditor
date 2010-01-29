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
* Video Editor Engine Test DLL.
*
*/


/* Choose proper header file, depending on which test framework 
   the tests are built for */

#ifndef VEDTESTTOP_H
#define VEDTESTTOP_H

#ifdef USING_CPPUNIT_FRAMEWORK

 /* CppUnit headers */
#include <CppUnit/Test.h>
#include <CppUnit/TestCase.h>
#include <CppUnit/TestCaller.h>
#include <CppUnit/TestSuite.h>

#else

  /* STIF TFW headers */
//#include "TestFramework/test.h"
//#include "TestFramework/TestCase.h"
//#include "TestFramework/TestCaller.h"
//#include "TestFramework/TestSuite.h"

#endif


//////////////////
// Utility function
//////////////////

void AddDriveLetterToPath(const TDesC &aFileName,TDes &aFileNameWithPath ) ;

#endif

