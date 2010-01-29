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
* Header for list.cpp.
*
*/


#ifndef _LIST_H_
#define _LIST_H_


/*
 * Defines
 */

#define LST_ITEM_SKELETON \
   void *next;

/*
 * Typedefs
 */

/* A skeleton of a list item type. 
   Each real list item type must include these definitions in the beginning 
   of the structure. This structure has no use as such outside this module. */
typedef struct lstListItem_s {
   void *next;
} lstListItem_t;

/* FIFO list */
typedef struct {
   lstListItem_t *head;
   lstListItem_t *tail;
   int numItems;
} fifo_t;

/* LIFO list (stack) */
typedef struct {
   lstListItem_t *head;
} lifo_t;

/* General linked list */
typedef struct {
   lstListItem_t *head;
   lstListItem_t *prev;
   lstListItem_t *curr;
   int numItems;
} lst_t;


/*
 * Function prototypes
 */

/* FIFO list functions */
int fifoOpen(fifo_t *list);
int fifoClose(fifo_t *list);
int fifoPut(fifo_t *list, void *item);
int fifoGet(fifo_t *list, void **item);
int fifoPeek(fifo_t *list, void **item);
#define fifoNumItems(list) ((list)->numItems)

/* LIFO list functions */
int lifoOpen(lifo_t *list);
int lifoClose(lifo_t *list);
int lifoPut(lifo_t *list, void *item);
int lifoGet(lifo_t *list, void **item);

/* General linked list functions */
int lstOpen(lst_t *list);
int lstClose(lst_t *list);
int lstHead(lst_t *list, void **item);
int lstTail(lst_t *list, void **item);
int lstEnd(lst_t *list);
int lstCurr(lst_t *list, void **item);
int lstNext(lst_t *list, void **item);
int lstNextExists(lst_t *list);
int lstAdd(lst_t *list, void *item);
int lstRemove(lst_t *list, void **item);
#define lstNumItems(list) ((list)->numItems)

#endif
// End of File
