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
* A header for debug output functions.
*
*/


#ifndef _DEBUG_H_
#define _DEBUG_H_

#if defined(DEB_FILE) || defined(DEB_STDOUT) || defined(DEB_DEBUGGER)
   /* Debug output wanted */

   #include <stdio.h>
   #include "nrctypes.h"

   #ifdef DEBUG_OUTPUT
   #include "biblin.h"
   #endif

   #ifdef __cplusplus
   extern "C" {
   #endif

   /*
    * Function prototypes
    */

   int debFree(void);
   int debLoad(const char *fileName);

   #ifdef DEB_PERF
      #define deb(dummy) 0
      #define debp(dummy) 0
      #define debPrintf(dummy) 0
      #define deb0p(format) 0
      #define deb1p(format, p1) 0
      #define deb2p(format, p1, p2) 0
      #define deb3p(format, p1, p2, p3) 0
      #define deb4p(format, p1, p2, p3, p4) 0
      #define deb5p(format, p1, p2, p3, p4, p5) 0
      #define deb0f(stream, format) 0
      #define deb1f(stream, format, p1) 0
      #define deb2f(stream, format, p1, p2) 0
      #define deb3f(stream, format, p1, p2, p3) 0
      #define deb4f(stream, format, p1, p2, p3, p4) 0
      #define deb5f(stream, format, p1, p2, p3, p4, p5) 0

      #ifdef DEB_STDOUT
         #define debPerf printf
      #else
         int debPerf(const char *format, ...);
      #endif

   #else
      #ifdef DEB_STDOUT
         #define deb printf
      #else
      #ifdef DEBUG_OUTPUT
         int deb_core(const char *format, ...);
         #define deb deb_core("%08lu: ", bibNumberOfFlushedBits(buffer_global)), deb_core
      #else
         int deb(const char *format, ...);
      #endif
      #endif

      #define debp deb

      #ifdef DEB_FILELINE
         #define debPrintf deb("%s, line %d. ", __FILE__, __LINE__), deb
      #else
         #define debPrintf deb
      #endif

      #define deb0p debPrintf
      #define deb1p debPrintf
      #define deb2p debPrintf
      #define deb3p debPrintf
      #define deb4p debPrintf
      #define deb5p debPrintf

      /* Internal defines to enable DEB_FILELINE */
      #ifdef DEB_STDOUT
         #define deb0ff(stream, format) fprintf(stream, format)
         #define deb1ff(stream, format, p1) fprintf(stream, format, p1)
         #define deb2ff(stream, format, p1, p2) fprintf(stream, format, p1, p2)
         #define deb3ff(stream, format, p1, p2, p3) fprintf(stream, format, p1, p2, p3)
         #define deb4ff(stream, format, p1, p2, p3, p4) fprintf(stream, format, p1, p2, p3, p4)
         #define deb5ff(stream, format, p1, p2, p3, p4, p5) fprintf(stream, format, p1, p2, p3, p4, p5)
      #else
         #define deb0ff(stream, format) deb(format)
         #define deb1ff(stream, format, p1) deb(format, p1)
         #define deb2ff(stream, format, p1, p2) deb(format, p1, p2)
         #define deb3ff(stream, format, p1, p2, p3) deb(format, p1, p2, p3)
         #define deb4ff(stream, format, p1, p2, p3, p4) deb(format, p1, p2, p3, p4)
         #define deb5ff(stream, format, p1, p2, p3, p4, p5) deb(format, p1, p2, p3, p4, p5)
      #endif

      #ifdef DEB_FILELINE
         #define deb0f(stream, format) \
            deb2ff(stream, "%s, line %d. ", __FILE__, __LINE__), \
            deb0ff(stream, format)

         #define deb1f(stream, format, p1) \
            deb2ff(stream, "%s, line %d. ", __FILE__, __LINE__), \
            deb1ff(stream, format, p1)

         #define deb2f(stream, format, p1, p2) \
            deb2ff(stream, "%s, line %d. ", __FILE__, __LINE__), \
            deb2ff(stream, format, p1, p2)

         #define deb3f(stream, format, p1, p2, p3) \
            deb2ff(stream, "%s, line %d. ", __FILE__, __LINE__), \
            deb3ff(stream, format, p1, p2, p3)

         #define deb4f(stream, format, p1, p2, p3, p4) \
            deb2ff(stream, "%s, line %d. ", __FILE__, __LINE__), \
            deb4ff(stream, format, p1, p2, p3, p4)

         #define deb5f(stream, format, p1, p2, p3, p4, p5) \
            deb2ff(stream, "%s, line %d. ", __FILE__, __LINE__), \
            deb4ff(stream, format, p1, p2, p3, p4, p5)

      #else
         #define deb0f(stream, format) deb0ff(stream, format)
         #define deb1f(stream, format, p1) deb1ff(stream, format, p1)
         #define deb2f(stream, format, p1, p2) deb2ff(stream, format, p1, p2)
         #define deb3f(stream, format, p1, p2, p3) deb3ff(stream, format, p1, p2, p3)
         #define deb4f(stream, format, p1, p2, p3, p4) deb4ff(stream, format, p1, p2, p3, p4)
         #define deb5f(stream, format, p1, p2, p3, p4, p5) deb5ff(stream, format, p1, p2, p3, p4, p5)
      #endif

      #define debPerf deb
   #endif

   #ifdef __cplusplus
   };
   #endif

#else /* no debug output wanted */

   #define deb(dummy) 
   #define debp(dummy) 
   #define debPrintf(dummy) 
   #define deb0p(format) 
   #define deb1p(format, p1) 
   #define deb2p(format, p1, p2) 
   #define deb3p(format, p1, p2, p3) 
   #define deb4p(format, p1, p2, p3, p4) 
   #define deb5p(format, p1, p2, p3, p4, p5) 
   #define deb0f(stream, format) 
   #define deb1f(stream, format, p1) 
   #define deb2f(stream, format, p1, p2) 
   #define deb3f(stream, format, p1, p2, p3) 
   #define deb4f(stream, format, p1, p2, p3, p4) 
   #define deb5f(stream, format, p1, p2, p3, p4, p5) 
   #define debPerf(dummy) 
   #define debFree() 
   #define debLoad(dummy) 

#endif

   #define debLogOutput(a,b,c) 

#endif /* !defined(_DEBUG_H_) */

// End of file
