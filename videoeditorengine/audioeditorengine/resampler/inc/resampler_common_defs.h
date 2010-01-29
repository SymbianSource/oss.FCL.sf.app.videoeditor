#ifndef __RESAMPLER_COMMON_DEFS_H__
#define __RESAMPLER_COMMON_DEFS_H__
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



/* Definitions for the FORCEINLINE macro. This is makes some of the 
 * optimized versions far more easy to read.
 */

#if (!defined(__WINSCW__))
#define FORCEINLINE __forceinline

#else
#define FORCEINLINE inline

#endif // __WINSCW__

/* Definitions for the packed struct.
 */
#if (!defined(__WINSCW__))
#undef  PACKED_RVCT
#define PACKED_RVCT  __packed
#undef  PACKED_GCC
#define PACKED_GCC

#else
#undef  PACKED_RVCT
#define PACKED_RVCT
#undef  PACKED_GCC
#define PACKED_GCC

#endif // __WINSCW__

/* Definitions for the min and max macros */
#define EAP_MAX(a, b) (((a)>=(b)) ? (a) : (b))
#define EAP_MIN(a, b) (((a)<(b)) ? (a) : (b))

#undef FLT_MAX
#define FLT_MAX  3.40282347e+38F


/* Handle stuff. */

/** Declare a handle type corresponding to type @c aType.
 *
 *  For example, calling this macro with @c aType being @c X declares
 *  handle type @c XHandle. */
/* Defining handle types this way is safer than just typedef'ing void
 * pointers, because the compiler is then able to catch more kinds of
 * handle misusage. */
#define DECLARE_HANDLE_TYPE(aType)                     \
        struct aType##HandleTag { int m_dummy; };      \
        typedef struct aType##HandleTag* aType##Handle

/** @return A pointer to the instance of type @c aType, and associated
 *          with @c aHandle. */
#define HANDLE_TO_PINST(aType, aHandle) ((aType*)aHandle)

/** @return A handle associated with the pointer to the instance of type
 *          @c aType. */
#define PINST_TO_HANDLE(aType, pInstance) ((aType##Handle)pInstance)

#endif  /* __RESAMPLER_COMMON_DEFS_H__ */
