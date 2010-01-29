#ifndef __RESAMPLER_DATA_TYPES_H__
#define __RESAMPLER_DATA_TYPES_H__
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


#include <limits.h>
#include <stddef.h>

/* Disable some warnings so that code compiles cleanly with highest 
 * warning levels. These are all related to function inlining.
 */


/* Disable warnings for Windows and WINS compilations */
#if defined(_MSC_VER)
/* unreferenced inline function has been removed */
#pragma warning(disable: 4514) 

/* function not inlined */
#pragma warning(disable: 4710) 

/* selected for automatic inline expansion */
#pragma warning(disable: 4711)
#endif

/** @ingroup types

@file resampler_data_types.h

A header file for sized integer types.


*/

typedef signed short int        int16;
typedef signed long int         int24;
typedef signed int              int32;

typedef signed long long int    int40;
typedef signed long long int    int48;
typedef signed long long int    int64;

typedef unsigned short int      uint16;
typedef unsigned long int       uint24;
typedef unsigned int            uint32;

typedef unsigned long long int  uint40;
typedef unsigned long long int  uint48;
typedef unsigned long long int  uint64;

#define RESAMPLER_INT16_MAX        ((int16)32767)
#define RESAMPLER_INT16_MIN ((int16)(-32767 - 1))
#define RESAMPLER_INT24_MAX             (8388607)
#define RESAMPLER_INT24_MIN        (-8388607 - 1)
#define RESAMPLER_INT32_MAX          (2147483647)
#define RESAMPLER_INT32_MIN     (-2147483647 - 1)


typedef enum RESAMPLER_DataType
{
    /** Sixteen-bit data type (Q15) */
    RESAMPLER_DATA_TYPE_INT16 = 0,
    /** Twenty-four-bit data type (Q19) */
    RESAMPLER_DATA_TYPE_INT24,
    /** Thirty-two-bit data type (Q23) */
    RESAMPLER_DATA_TYPE_INT32,
    /** Thirty-two-bit data type (Q15) */
    RESAMPLER_DATA_TYPE_INT32_Q15,
    /** Floating-point data type */
    RESAMPLER_DATA_TYPE_FLOAT,
    /** Number of different data types */
    RESAMPLER_DATA_TYPE_COUNT
} RESAMPLER_DataType;

/** @return A string that describes @c aType.
 *
 *  @pre @c aType is a valid instance of @c RESAMPLER_DataType. */
const char*  RESAMPLER_DataType_AsString(RESAMPLER_DataType aType);

/** @return The @c RESAMPLER_TestDataType that corresponds to @c pString.
 *
 *  @pre @c pString is equal to "int16", "int24", "int32", "int32Q15", or "float. */
RESAMPLER_DataType RESAMPLER_DataType_FromString(const char* pString);

/** @return Whether @c aType is a valid instance of
 *          @c RESAMPLER_DataType. */
int          RESAMPLER_DataType_IsValid(RESAMPLER_DataType aType);

/** @return The size, in bytes, of an element of type @c aType.
 *
 *  @pre @c aType is a valid instance of @c RESAMPLER_DataType. */
size_t       RESAMPLER_DataType_Size(RESAMPLER_DataType aType);

#endif  /* __RESAMPLER_DATA_TYPES_H__ */
