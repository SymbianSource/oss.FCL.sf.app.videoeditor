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
* Doubly linked list module.
*
*/


/*
 * dlist.c
 *
 *    
 *
 * Project:
 *    Mobile Videophone Demonstrator
 *    H.263 Video Decoder
 *
 * Contains:
 *    This module contains generic list handling functions for doubly linked 
 *    lists, i.e. one can use the same set of functions regardless of the type 
 *    of list items.
 *    The list item type has a constraint that it must begin with the skeleton
 *    of a list item (see the definition of DLST_ITEM_SKELETON in dlist.h).
 *
 *    The prefix ddlst is used in common names defined in this module.
 *
 *
 *
 */


#include "dlist.h"


#ifndef NULL
   #define NULL 0
#endif


/*
 * dlstClose
 *    
 *
 * Parameters:
 *    list                       a pointer to a list
 *
 * Function:
 *    This function deinitializes the given list. Notice that no dynamic
 *    allocation for the list is done. This function must be called when
 *    the list is no longer used.
 *
 * Returns:
 *    >= 0 if the function was successful
 *    < 0  indicating an error
 *
 */

int dlstClose(dlst_t *list)
{
   list->head = list->tail = list->curr = NULL;
   list->numItems = 0;

   return 0;
}


/*
 * dlstRemove
 *    
 *
 * Parameters:
 *    list                       a pointer to a list
 *    item                       used to return a pointer to the removed
 *                               list item
 *
 * Function:
 *    This function removes an item from the list. Notice that no copying of
 *    contents of the given item is done, i.e. only the pointer to the item
 *    is used.
 *
 *    If there are no items left in the list, NULL will be returned in
 *    the item parameter.
 *
 * Returns:
 *    >= 0 if the function was successful
 *    < 0  indicating an error
 *
 */

int dlstRemove(dlst_t *list, void **item)
{
   if (list->curr) {
      dlstListItem_t 
         *curr = list->curr, 
         *prev = (dlstListItem_t *) (list->curr->prev),
         *next = (dlstListItem_t *) (list->curr->next);

      *item = (void *) curr;

      if (prev)
         prev->next = curr->next;
      else
         list->head = (dlstListItem_t *) curr->next;

      if (next)
         next->prev = curr->prev;

      curr->next = NULL;
      curr->prev = NULL;

      if (curr == list->tail) {
         list->tail = prev;
         list->curr = prev;
      }
      else
         list->curr = next;

      list->numItems--;
   }

   else
      *item = NULL;

   return 0;
}


/*
 * dlstOpen
 *    
 *
 * Parameters:
 *    list                       a pointer to a list
 *
 * Function:
 *    This function initializes the given list. Notice that no dynamic
 *    allocation for the list is done. This function must be called before
 *    the list is used.
 *
 * Returns:
 *    >= 0 if the function was successful
 *    < 0  indicating an error
 *
 */

int dlstOpen(dlst_t *list)
{
   list->head = list->tail = list->curr = NULL;
   list->numItems = 0;

   return 0;
}


/*
 * dlstHead
 *    
 *
 * Parameters:
 *    list                       a pointer to a dlst list
 *    item                       used to return a pointer to the head item
 *                               of the list
 *
 * Function:
 *    This function sets the current access point to the head of the list and
 *    returns a pointer to the head item. Notice that no copying of
 *    contents of the given item is done, i.e. only the pointer to the item
 *    is used.
 *
 *    If there are no items left in the list, NULL will be returned in
 *    the item parameter.
 *
 * Returns:
 *    >= 0 if the function was successful
 *    < 0  indicating an error
 *
 */

int dlstHead(dlst_t *list, void **item)
{
   if (list->head)
      *item = (void *) list->head;

   else
      *item = NULL;

   list->curr = list->head;

   return 0;
}


/*
 * dlstTail
 *    
 *
 * Parameters:
 *    list                       a pointer to a dlst list
 *
 * Function:
 *    This function sets the current access point to the tail of the list
 *    enabling the item addition to the end of the list.
 *
 * Returns:
 *    >= 0 if the function was successful
 *    < 0  indicating an error
 *
 */

int dlstTail(dlst_t *list, void **item)
{

   if (list->tail)
      *item = (void *) list->tail;

   else
      *item = NULL;

   list->curr = list->tail;

   return 0;
}


/*
 * dlstNext
 *    
 *
 * Parameters:
 *    list                       a pointer to a dlst list
 *    item                       used to return a pointer to the next item
 *                               of the list
 *
 * Function:
 *    This function sets the current access point to the next item of the list
 *    and returns a pointer to the item. Notice that no copying of
 *    contents of the given item is done, i.e. only the pointer to the item
 *    is used.
 *
 *    If there are no items left in the list, NULL will be returned in
 *    the item parameter and the current access point shall be left as it is.
 *
 * Returns:
 *    >= 0 if the function was successful
 *    < 0  indicating an error
 *
 */

int dlstNext(dlst_t *list, void **item)
{
   if (list->curr && list->curr->next) {
      list->curr = (dlstListItem_t *) (list->curr->next);
      *item = (void *) list->curr;
   }

   else
      *item = NULL;

   return 0;
}


int dlstPrev(dlst_t *list, void **item)
{
   if (list->curr && list->curr->prev) {
      list->curr = (dlstListItem_t *) (list->curr->prev);
      *item = (void *) list->curr;
   }

   else
      *item = NULL;

   return 0;
}


int dlstCurr(dlst_t *list, void **item)
{
   *item = (void *) list->curr;

   return 0;
}


int dlstNextExists(dlst_t *list)
{
   return (list->curr && list->curr->next) ? 1 : 0;
}

/*
 * dlstAdd
 *    
 *
 * Parameters:
 *    list                       a pointer to a dlst list
 *    item                       an item to add to the list
 *
 * Function:
 *    This function adds an item into the list. Notice that no copying of
 *    contents of the given item is done, i.e. only the pointer to the item
 *    is used.
 *
 * Returns:
 *    >= 0 if the function was successful
 *    < 0  indicating an error
 *
 */

int dlstAddBeforeCurr(dlst_t *list, void *item)
{
   dlstListItem_t
      *curr, *prev, *listItem = (dlstListItem_t *) item;

   if (list->curr) {
      curr = (dlstListItem_t *) list->curr, 
      prev = (dlstListItem_t *) list->curr->prev;

      if (prev)
         prev->next = item;
      else
         list->head = (dlstListItem_t *) item;
      listItem->prev = prev;

      curr->prev = item;
      listItem->next = curr;

      list->curr = (dlstListItem_t *) item;
   }

   else {
      list->head = list->tail = list->curr = (dlstListItem_t *) item;
      listItem->next = NULL;
      listItem->prev = NULL;
   }

   list->numItems++;

   return 0;
}

int dlstAddAfterCurr(dlst_t *list, void *item)
{
   dlstListItem_t
      *curr, *next, *listItem = (dlstListItem_t *) item;

   if (list->curr) {
      curr = list->curr, 
      next = (dlstListItem_t *) (list->curr->next);

      curr->next = item;
      listItem->prev = curr;

      if (next)
         next->prev = item;
      else
         list->tail = (dlstListItem_t *) item;
      listItem->next = next;

      list->curr = (dlstListItem_t *) item;
   }

   else {
      list->head = list->tail = list->curr = (dlstListItem_t *) item;
      listItem->next = NULL;
      listItem->prev = NULL;
   }

   list->numItems++;

   return 0;
}
// End of File
