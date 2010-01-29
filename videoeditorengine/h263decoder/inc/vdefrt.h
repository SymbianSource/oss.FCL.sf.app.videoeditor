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
* A header file for vdefrt.c.
*
*/


#ifndef _VDEFRT_H_
#define _VDEFRT_H_


/*
 * Includes
 */


#include "h263dapi.h"
#include "list.h"


/*
 * Defines
 */

/*
 * Structures and typedefs
 */

/* {{-output"vdeFrtStore_t_info.txt" -ignore"*" -noCR}}
   This structure is used to store the internal state of one frame type
   container instance. A pointer to this structure is passed to 
   the VDE Frame Type Container module functions to indicate the container
   instance which the functions should handle.
   {{-output"vdeFrtStore_t_info.txt"}} */

/* {{-output"vdeFrtStore_t.txt"}} */
typedef struct {
   lst_t sizeList;
} vdeFrtStore_t;
/* {{-output"vdeFrtStore_t.txt"}} */


/*
 * Function prototypes
 */

int vdeFrtClose(vdeFrtStore_t *store);

int vdeFrtGetItem(vdeFrtStore_t *store, h263dFrameType_t *frameType, 
   u_int32 *item);

int vdeFrtOpen(vdeFrtStore_t *store);

int vdeFrtPutItem(vdeFrtStore_t *store, h263dFrameType_t *frameType, 
   u_int32 item, void (*removeItem) (void *));

int vdeFrtRemoveItem(vdeFrtStore_t *store, h263dFrameType_t *frameType);

#endif
// End of File
