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
* Definitions to port vedh263d to Symbian.
*
*/


#ifndef _H263DCONFIG_H_
#define _H263DCONFIG_H_

#include "h263dext.h"

#if !defined(_DEBUG) && !defined(NDEBUG)
   #define _DEBUG
#endif



/* C standard libraries 

   header file    functions/macros/identifiers used
   ---------------------------------------------------------------------------
   assert.h       assert
   string.h       memcpy, memset
   stdlib.h       NULL, free, malloc, calloc, abs
   */

#if defined(__SYMBIAN32__)
#define __EPOC__
#include "epoclib.h"
#else
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#endif

/* Memory allocation wrappers for large (> 64 kB) memory chunks.
   Used in core, block and convert. Needed when compiling in
   systems with segmented memory (DOS, Windows 3.1) */
#define HUGE
#define MEMCPY memcpy
#define MEMSET memset
#define MALLOC malloc
#define FREE free


/* core.h */
#define VDC_INLINE __inline

/* debug */
/* Controlled from the project settings or makefile */

/* msgwrap etc. */
#ifndef BUF_DS
   #define BUF_DS
#endif

#ifdef __MARM__
   #define __cdecl
   #define __stdcall
#endif

#endif 
// End of File
