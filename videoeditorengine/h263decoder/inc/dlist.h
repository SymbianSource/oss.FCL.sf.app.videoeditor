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
* Header file for generic list handling services
* for doubly linked lists.
*
*/


#ifndef _DLIST_H_
#define _DLIST_H_

/*
 * Includes
 */

#include "list.h"


/*
 * Defines
 */

#define DLST_ITEM_SKELETON \
   LST_ITEM_SKELETON \
   void *prev;


/*
 * Typedefs
 */

/* A skeleton of a list item type. 
   Each real list item type must include these definitions in the beginning 
   of the structure. This structure has no use as such outside this module. */
typedef struct dlstListItem_s {
   DLST_ITEM_SKELETON
} dlstListItem_t;

/* Doubly-linked list */
typedef struct {
   dlstListItem_t *head;
   dlstListItem_t *curr;
   dlstListItem_t *tail;
   int numItems;
} dlst_t;


/*
 * Function prototypes
 */

/* Double-linked list functions */
int dlstOpen(dlst_t *list);
int dlstClose(dlst_t *list);
int dlstHead(dlst_t *list, void **item);
int dlstTail(dlst_t *list, void **item);
int dlstNext(dlst_t *list, void **item);
int dlstPrev(dlst_t *list, void **item);
int dlstCurr(dlst_t *list, void **item);
int dlstNextExists(dlst_t *list);
int dlstAddAfterCurr(dlst_t *list, void *item);
int dlstAddBeforeCurr(dlst_t *list, void *item);
int dlstRemove(dlst_t *list, void **item);
#define dlstNumItems(list) ((list)->numItems)

#endif
// End of File
