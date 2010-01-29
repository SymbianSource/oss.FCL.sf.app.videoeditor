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


/**************************************************************************
  defines.h - Global data prototypes, compile switches etc.

  Author(s): Juha Ojanpera
  Copyright (c) 1999-2003 by Nokia Research Center, Speech and Audio Systems.
  *************************************************************************/

#ifndef DEFINES_H_
#define DEFINES_H_

#ifdef NOT_SYMBIAN_OS

/*-- Header files to be included. --*/
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*-- Data types. --*/
typedef short int16;
typedef unsigned short uint16;
typedef float FLOAT;
#ifdef NATIVE64
typedef unsigned int uint32;
typedef int int32;
#else
typedef unsigned long uint32;
typedef long int32;
#endif
typedef int BOOL;
typedef unsigned char uint8;

#ifdef HAS_INLINE
#define INLINE inline
#else
#ifdef HAS_C_INLINE
#define INLINE __inline
#else
#define INLINE
#endif
#endif

#define GET_CHUNK(x) (memset(malloc(x), 0, x))
#define SWAP_INT16(x) ((((uint32)x >> 8) & 0xFF) | ((uint32)x << 8))
#define SAFE_DELETE(x) if(x) free(x); x = NULL
#define IS_ERROR(x) if(x == NULL) goto error_exit;
#define ZERO_MEMORY(x, y)    memset(x, 0, y)
#define COPY_MEMORY(x, y, z) memcpy(x, y, z)
#define SET_MEMORY(x, y, z)  memset(x, z, y)

#else

/*-- System Headers. --*/
#include <e32def.h>
#include <e32std.h>
#include <e32base.h>

//#include <stdlib.h>
//#include <stdio.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/*-- Data types. --*/
typedef TInt8             int8;
typedef TUint8            uint8;
typedef TInt16            int16;
typedef TUint16           uint16;
typedef TInt              int32;
typedef TUint             uint32;
typedef TUint8            BOOL;
typedef TReal           FLOAT;

#define INLINE inline

#define GET_CHUNK(x)         User::LeaveIfNull(User::Alloc(x))
#define GET_SYMBIAN_CHUNK(x)    x::NewL()

#define IS_ERROR(x)
#define SAFE_DELETE(x)       if(x) User::Free(x); x = NULL
#define SAFE_SYMBIAN_DELETE(x)    if(x != 0) delete x; x = 0

#define ZERO_MEMORY(x, y)    Mem::FillZ(x, y)
#define COPY_MEMORY(x, y, z) Mem::Copy(x, y, z)
#define SET_MEMORY(x, y, z)  Mem::Fill(x, y, TChar((TUint) z))


/*-- AAC definitions. --*/
#define LTP_PROFILE
#define MONO_VERSION
#define STEREO_VERSION
#define AAC_ADIF_FORMAT
#define AAC_ADTS_FORMAT
#define AAC_INDEX_TABLES

#define FAST_MIX


#endif /*-- NOT_SYMBIAN_OS --*/

#ifndef TRUE
#define TRUE  (1)
#endif
#ifndef FALSE
#define FALSE (0)
#endif

/*-- Some Useful Macros. --*/
#ifdef MIN
#undef MIN
#endif
#define MIN(A, B)            ((A) < (B) ? (A) : (B))

#ifdef MAX
#undef MAX
#endif
#define MAX(A, B)            ((A) > (B) ? (A) : (B))

#endif /*-- DEFINES_H_ --*/
