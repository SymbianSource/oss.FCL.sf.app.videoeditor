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
* Header file for epoclib.cpp.
*
*/


#ifndef _EPOC_LIB
#define _EPOC_LIB

#include <e32base.h>
#include <s32file.h>

#ifdef _DEBUG
#define MALLOC_DEBUG
#endif

#define EPOCLIB_DS

//#define FILE RFile
//#define void TAny

//specific to this program
//#define HANDLE void *

typedef TUint8   Byte;
typedef TUint8   u_char;
typedef TInt8    int8;
typedef TUint8   u_int8;
typedef TInt16   int16;
typedef TUint16  u_int16;
typedef TInt32   int32;
typedef TUint32  u_int32;
typedef TReal32  float32;
typedef TReal64  float64;


typedef unsigned int size_t;  


#define max(a,b)            (((a) > (b)) ? (a) : (b))
#define min(a,b)            (((a) < (b)) ? (a) : (b))

#define abs     Abs
#define labs    Abs


EPOCLIB_DS void free(TAny *ptr);

#ifdef MALLOC_DEBUG
#define malloc(size) debugMalloc(size, __FILE__, __LINE__)
EPOCLIB_DS TAny *debugMalloc(u_int32 size, char *file, int line);
#else
EPOCLIB_DS TAny *malloc(u_int32 size);
#endif
EPOCLIB_DS TAny *realloc(void *memblock, u_int32 size);
#ifdef MALLOC_DEBUG
#define calloc(num, size) debugCalloc(num, size, __FILE__, __LINE__)
EPOCLIB_DS TAny *debugCalloc(u_int32 num, u_int32 size, char *file, int line);
#else
EPOCLIB_DS TAny *calloc(u_int32 num, u_int32 size);
#endif
EPOCLIB_DS TAny *memset(TAny *dest, TInt c, TInt size); 
EPOCLIB_DS TAny *memcpy(TAny *dest, const TAny *src, TInt size);
EPOCLIB_DS TAny *memmove(TAny *dest, const TAny *src, u_int32 count);

#define assert(expr) __ASSERT_DEBUG(expr, User::Panic(_L("assert"), 1));
EPOCLIB_DS int atoi(const char *nptr);


EPOCLIB_DS u_int32 fwrite(const TAny *buffer, u_int32 size, u_int32 count, RFile stream);

#endif



// End of File
