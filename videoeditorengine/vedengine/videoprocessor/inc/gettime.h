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
* Header for gettime.cpp.
*
*/

#ifndef _GETTIME_H_
#define _GETTIME_H_


/*
 * Includes
 */

#include "nrctyp32.h"
#include "thdwrap.h"


/*
 * Definitions
 */

/* Number of output ticks (from gtGetTime) per second */
#ifndef GT_NUM_OUT_TICKS_PER_SEC
   #define GT_NUM_OUT_TICKS_PER_SEC 10000
#endif

/* Maximum value returned by gtGetTime */
#define GT_TIME_MAX 0xffffffff

/* If TMR_DS not defined, define it as nothing */
#ifndef TMR_DS
#ifdef __PSISOFT32__
    #define TMR_DS
#else
   #define TMR_DS
#endif
#endif

/*
 * Function prototypes
 */


TMR_DS void gtInitialize( void );

//TMR_DS TInt gtGetTime( array_t *array );
TMR_DS unsigned int gtGetTime( void );


#endif
// End of File
