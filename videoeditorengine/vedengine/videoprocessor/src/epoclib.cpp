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
* Wrappers for Symbian OS -specific system functions.
*
*/


#include <e32base.h>
#include <s32file.h>
#include "nrctyp32.h"
#include "epoclib.h"

#define NAMELEN 120
#define EL_EXPORT


EL_EXPORT void free(TAny *ptr)

    {
#ifdef MALLOC_DEBUG        
    if ( !ptr )
        return;

    unsigned *p32 = (unsigned*) (((unsigned) ptr) - (NAMELEN+8));

    if ( p32[0] != 0xdaded1d0 )
        User::Panic(_L("BadFree"), 1);

    ptr = (TAny*) p32;
#endif
    
    //  Dangling pointer check
//    User::Free(ptr);
    unsigned count = (unsigned) User::AllocLen(ptr);
    TUint8 *p;
    
    p = (TUint8*) ptr;
    while ( count >> 2 )
    {
        *((TUint32*)p) = 0xfeedf1d0;
        p += 4;
        count -= 4;
    }
    while ( count )
    {
        *(p++) = 0x17;
        count--;
    }

    User::Free(ptr);
    }

#ifdef MALLOC_DEBUG
EL_EXPORT TAny *debugMalloc(u_int32 size, char *file, int line)
#else    
EL_EXPORT TAny *malloc(u_int32 size)
#endif    
    {
    // Uninit checks        
//    return User::AllocL(size);
    TAny *ptr;
    unsigned count = size;
    TUint8 *p;

#ifdef MALLOC_DEBUG
    ptr = User::AllocL(size + NAMELEN+8);
    p = ((TUint8*) ptr) + NAMELEN+8;
#else
    ptr = User::AllocL(size);
    p = (TUint8*) ptr;
#endif    
    
    while ( count >> 2 )
    {
        *((TUint32*)p) = 0xdeadbeef;
        p += 4;
        count -= 4;
    }
    while ( count )
    {
        *(p++) = 0x42;
        count--;
    }

#ifdef MALLOC_DEBUG
    unsigned *p32 = (unsigned*) ptr;
    p32[0] = 0xdaded1d0;
    p32[1] = (unsigned) line;
    char *c = (char*) &p32[2];
    int n = NAMELEN;
    while ( (*file != 0) && n )
    {
        *(c++) = *(file++);
        n--;
    }
    return (TAny*) (((unsigned) p32) + NAMELEN + 8);
#else  
    return ptr;
#endif    
    }

EL_EXPORT TAny *realloc(void *memblock, u_int32 size)
    {
#ifdef MALLOC_DEBUG
    unsigned *p32 = (unsigned*) (((unsigned) memblock) - NAMELEN - 8);

    if ( p32[0] != 0xdaded1d0 )
        User::Panic(_L("BadRealloc"), 1);

    p32 = (unsigned*) User::ReAllocL((void*) p32, size+NAMELEN+8);
    return (TAny*) (((unsigned) p32) + NAMELEN + 8);
#else
    return User::ReAllocL(memblock, size);
#endif    
    }


#ifdef MALLOC_DEBUG
EL_EXPORT TAny *debugCalloc(u_int32 num, u_int32 size, char *file, int line)
{
    TAny *ptr;
    TUint8 *p;

    ptr = User::Alloc(num*size + NAMELEN+8);
    if ( !ptr )
        return 0;
    p = ((TUint8*) ptr) + NAMELEN+8;
    
    Mem::Fill(p, size*num, 0);

    unsigned *p32 = (unsigned*) ptr;
    p32[0] = 0xdaded1d0;
    p32[1] = (unsigned) line;
    char *c = (char*) &p32[2];
    int n = NAMELEN;
    while ( (*file != 0) && n )
    {
        *(c++) = *(file++);
        n--;
    }
    return (TAny*) p;
}
#else    
EL_EXPORT TAny *calloc(u_int32 num, u_int32 size)

    {
    TAny *dest = User::Alloc(size*num);
    Mem::Fill(dest, size*num, 0);
    return dest;
    }
#endif

EL_EXPORT TAny *memset(TAny *dest, TInt c, u_int32 size)
    {
    Mem::Fill(dest, size, c);
    return dest; //returning the value of dest as in windows
    }
EL_EXPORT TAny *memcpy(TAny *dest, const TAny *src, u_int32 size)
    {
    Mem::Copy(dest, src, size);
    return dest;
    }

EL_EXPORT TAny *memmove(TAny *dest, const TAny *src, u_int32 count)
    {
    Mem::Copy(dest,src,count);
    return dest;
    }

long atol(
        const char *nptr
        )
{
        int c;              // current char 
        long total;         // current total 
        int sign;           // if '-', then negative, otherwise positive 

        TLex8 string((unsigned char *)nptr);
        // skip whitespace 
        string.SkipSpace();
        
        //prendre un caratere dans string lui faire le sign
        c = (int)string.Peek();
        string.Inc();

        sign = c;           // save sign indication 
        if (c == '-' || c == '+')
        // skip sign 
            {c = (int)string.Peek();
              string.Inc();
            }
        else //If c is not a sign, it is necessary to go increment back into the descriptors to get the right
             //number 
            {
            string.UnGet();
            }

        total = 0;

        while (string.Peek().IsDigit())
            {
            total = 10 * total + (c - '0');
            string.Inc();
            c = (int)string.Peek();
            
            }

        if (sign == '-')
            return -total;
        else
            return total;   /* return result, negated if necessary */
}


/*
*int atoi(char *nptr) - Convert string to long
*
*Purpose:
*       Converts ASCII string pointed to by nptr to binary.
*       Overflow is not detected.  Because of this, we can just use
*       atol().
*
*Entry:
*       nptr = ptr to string to convert
*
*Exit:
*       return int value of the string
*
*Exceptions:
*       None - overflow is not detected.
*
*******************************************************************************/

EL_EXPORT int atoi(
        const char *nptr
        )
{
        return (int)atol(nptr);
}
// End of File
