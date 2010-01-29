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
* Header file for the Video Decoder Engine image buffer store.
*
*/


#ifndef _VDEIMS_H_
#define _VDEIMS_H_

/*
 * Includes
 */

#include "list.h"
#include "vdeimb.h"

/* 
 * Defines
 */

/* for the mode parameter of vdeImsGetReference */
#define VDEIMS_REF_TR      0
#define VDEIMS_REF_LATEST  1
#define VDEIMS_REF_OLDEST  2


/*
 * Typedefs
 */

/* {{-output"vdeImsItem_t_info.txt" -ignore"*" -noCR}}
   The image store item structure is used throughout the Image Store
   module to store individual image buffers.
   {{-output"vdeImsItem_t_info.txt"}} */

/* {{-output"vdeImsItem_t.txt"}} */
typedef struct {
   LST_ITEM_SKELETON

   vdeImb_t *imb;             /* pointer to image buffer structure */
} vdeImsItem_t;
/* {{-output"vdeImsItem_t.txt"}} */


/* {{-output"vdeIms_t_info.txt" -ignore"*" -noCR}}
   vdeIms_t defines the structure of an image buffer store.
   A pointer to this structure is passed to the VDE Image Store module
   functions to indicate the image store instance which the functions should
   handle. One should not access the members of this structure directly
   but rather always use the provided access functions.
   {{-output"vdeIms_t_info.txt"}} */

/* {{-output"vdeIms_t.txt"}} */
typedef struct {
   lst_t freeStoreList;       /* a list of "Free" stores each of which is
                                 carrying frames with a certain (unique) 
                                 size */

   vdeImsItem_t *refFrame;    /* refFrame is a pointer to
                                 the latest reference (INTRA or INTER) frame */

   lst_t idleStore;           /* a list of non-referenced but to-be-displayed
                                 image store items */
   int fYuvNeeded;

} vdeIms_t;
/* {{-output"vdeIms_t.txt"}} */


/*
 * Function prototypes
 */

int vdeImsChangeReference(vdeIms_t *store, vdeImsItem_t *newItem);

int vdeImsClose(vdeIms_t *store);

int vdeImsDebRefStore(vdeIms_t *store);

int vdeImsFromRenderer(vdeIms_t *store, void *drawItem);

int vdeImsGetFree(vdeIms_t *store, int lumWidth, int lumHeight, 
   vdeImsItem_t **item);

int vdeImsGetReference(vdeIms_t *store, int mode, int tr, vdeImsItem_t **item);

int vdeImsOpen(vdeIms_t *store, int numFreeItems, int lumWidth, int lumHeight);

void vdeImsSetYUVNeed(vdeIms_t *store, int fYuvNeeded);

int vdeImsPut(vdeIms_t *store, vdeImsItem_t *item);

int vdeImsPutFree(vdeIms_t *store, vdeImsItem_t *item);

int vdeImsReleaseReference(vdeIms_t *store, vdeImsItem_t *item);



/*
 * Macros
 */

/* Note: vdeImsImageBufferToStoreItem and vdeImsStoreItemToImageBuffer
   need not be used since it is considered that the vdeImsItem_t structure
   is public and one can directly refer the image buffer to as pImsItem->imb.
   However, these macros exist to provide compatibility to some "old" code. */

/*
 * vdeImsImageBufferToStoreItem
 *
 * Parameters:
 *    vdeImb_t *buffer           a pointer to an image buffer
 *    vdeImsItem_t *storeItem    a pointer to a image store item
 *
 * Function:
 *    This macro associates the given image buffer with the given
 *    image store item.
 *
 * Returns:
 *    >= 0 if successful
 *    < 0 indicating an error
 *
 */

#define vdeImsImageBufferToStoreItem(buffer, storeItem) ((storeItem)->imb = (buffer), 0)


/*
 * vdeImsStoreItemToImageBuffer
 *
 * Parameters:
 *    vdeImsItem_t *storeItem    a pointer to a image store item
 *    vdeImb_t **buffer          a pointer to an image buffer pointer
 *
 * Function:
 *    This macro returns the pointer to the image buffer associated with
 *    the given image store item.
 *
 * Returns:
 *    >= 0 if successful
 *    < 0 indicating an error
 *
 */

#define vdeImsStoreItemToImageBuffer(storeItem, buffer) (*(buffer) = (storeItem)->imb, 0)


#endif
// End of File
