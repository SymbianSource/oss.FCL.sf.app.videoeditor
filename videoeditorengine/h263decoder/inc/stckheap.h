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
* Macros for stack/heap allocation.
*
*/


#ifndef _STCKHEAP_H_
#define _STCKHEAP_H_

/*
 * Include files
 */


/*
 * Internal macros
 * (Not supposed to be used in any other file)
 */

/* Concatenates two strings */
#define SOH_CAT_STRINGS(item1, item2) item1 ## item2

/* Concatenates two macros */
#define SOH_CAT_MACROS(macro1, macro2) SOH_CAT_STRINGS(macro1, macro2)


/* 
 * Public macros
 */

/*
 * SOH_DEFINE
 *
 * Parameters:
 *    type_t                     type name
 *    varName                    variable name
 *
 * Function:
 *    This macro defines a variable called varName of type type_t *.
 *
 * Example:
 *    SOH_DEFINE(int, pValue); defines a variable called pValue of type int *.
 */

/*
 * SOH_ALLOC
 *
 * Parameters:
 *    type_t                     type name
 *    varName                    variable name
 *
 * Function:
 *    This macro allocates and initializes a variable previously created
 *    with SOH_DEFINE.
 *
 * Example:
 *    SOH_ALLOC(int, pValue); allocates an int variable and sets pValue to
 *    point to it.
 */

/*
 * SOH_DEALLOC
 *
 * Parameters:
 *    varName                    variable name
 *
 * Function:
 *    This macro deallocates a variable previously created
 *    with SOH_DEFINE and SOH_ALLOC.
 *
 * Example:
 *    SOH_DEALLOC(pValue); deallocates pValue.
 */


/* Set the pointer to NULL in order to allow safe usage of SOH_DEALLOC
   without a call to SOH_ALLOC first. */
#define SOH_DEFINE(type_t, varName) \
   type_t *varName = NULL
  
#define SOH_ALLOC(type_t, varName) \
   varName = (type_t *) malloc(sizeof(type_t))
   
#define SOH_DEALLOC(varName) \
   free(varName)


#endif
// End of File
