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
* List handling functions.
*
*/



#include "h263dconfig.h"
#include "list.h"

#ifndef NULL
   #define NULL 0
#endif


/*
 * fifoClose
 *    
 *
 * Parameters:
 *    list                       a pointer to a FIFO list
 *
 * Function:
 *    This function deinitializes the given FIFO list. Notice that no dynamic
 *    allocation for the list is done. This function must be called when
 *    the list is no longer used.
 *
 * Returns:
 *    >= 0 if the function was successful
 *    < 0  indicating an error
 *
 */

int fifoClose(fifo_t *list)
{
   list->head = list->tail = NULL;
   list->numItems = 0;

   return 0;
}


/*
 * fifoGet
 *    
 *
 * Parameters:
 *    list                       a pointer to a FIFO list
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

int fifoGet(fifo_t *list, void **item)
{
   if (list->head) {
      *item = (void *) list->head;
      list->head = (lstListItem_t *) list->head->next;
      if (list->head == NULL)
         list->tail = NULL;
      list->numItems--;
   }

   else
      *item = NULL;

   return 0;
}


/*
 * fifoOpen
 *    
 *
 * Parameters:
 *    list                       a pointer to a FIFO list
 *
 * Function:
 *    This function initializes the given FIFO list. Notice that no dynamic
 *    allocation for the list is done. This function must be called before
 *    the list is used.
 *
 * Returns:
 *    >= 0 if the function was successful
 *    < 0  indicating an error
 *
 */

int fifoOpen(fifo_t *list)
{
   list->head = list->tail = NULL;
   list->numItems = 0;

   return 0;
}


/*
 * fifoPeek
 *    
 *
 * Parameters:
 *    list                       a pointer to a FIFO list
 *    item                       used to return a pointer to the removed
 *                               list item
 *
 * Function:
 *    This function returns a pointer to the next in the list but does not
 *    remove the item from the list. Notice that no copying of
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

int fifoPeek(fifo_t *list, void **item)
{
   if (list->head)
      *item = (void *) list->head;

   else
      *item = NULL;

   return 0;
}


/*
 * fifoPut
 *    
 *
 * Parameters:
 *    list                       a pointer to a FIFO list
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

int fifoPut(fifo_t *list, void *item)
{
   ((lstListItem_t *) item)->next = NULL;

   if (list->tail) {
      list->tail->next = item;
      list->tail = (lstListItem_t *) (item);
   }

   else
      list->head = list->tail = (lstListItem_t *) item;

   list->numItems++;

   return 0;
}


/*
 * lifoClose
 *    
 *
 * Parameters:
 *    list                       a pointer to a LIFO list
 *
 * Function:
 *    This function deinitializes the given LIFO list. Notice that no dynamic
 *    allocation for the list is done. This function must be called when
 *    the list is no longer used.
 *
 * Returns:
 *    >= 0 if the function was successful
 *    < 0  indicating an error
 *
 */

int lifoClose(lifo_t *list)
{
   list->head = NULL;

   return 0;
}


/*
 * lifoGet
 *    
 *
 * Parameters:
 *    list                       a pointer to a LIFO list
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

int lifoGet(lifo_t *list, void **item)
{
   if (list->head) {
      *item = list->head;
      list->head = (lstListItem_t *) list->head->next;
   }
   else
      *item = NULL;

   return 0;
}


/*
 * lifoOpen
 *    
 *
 * Parameters:
 *    list                       a pointer to a LIFO list
 *
 * Function:
 *    This function initializes the given LIFO list. Notice that no dynamic
 *    allocation for the list is done. This function must be called before
 *    the list is used.
 *
 * Returns:
 *    >= 0 if the function was successful
 *    < 0  indicating an error
 *
 */

int lifoOpen(lifo_t *list)
{
   list->head = NULL;

   return 0;
}


/*
 * lifoPut
 *    
 *
 * Parameters:
 *    list                       a pointer to a LIFO list
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

int lifoPut(lifo_t *list, void *item)
{
   ((lstListItem_t *) item)->next = list->head;
   list->head = (lstListItem_t *) item;

   return 0;
}


/*
 * lstClose
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

int lstClose(lst_t *list)
{
   list->head = list->prev = list->curr = NULL;
   list->numItems = 0;

   return 0;
}


/*
 * lstRemove
 *    
 *
 * Parameters:
 *    list                       a pointer to a lst list
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

int lstRemove(lst_t *list, void **item)
{
   if (list->curr) {
      *item = (void *) list->curr;
      if (list->prev)
         list->prev->next = list->curr->next;
      else
         list->head = (lstListItem_t *) (list->curr->next);
      list->curr = (lstListItem_t *) (list->curr->next);
      ((lstListItem_t *) *item)->next = NULL;
      list->numItems--;
   }

   else
      *item = NULL;

   return 0;
}


/*
 * lstOpen
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

int lstOpen(lst_t *list)
{
   list->head = list->prev = list->curr = NULL;
   list->numItems = 0;

   return 0;
}


/*
 * lstHead
 *    
 *
 * Parameters:
 *    list                       a pointer to a lst list
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

int lstHead(lst_t *list, void **item)
{
   if (list->head)
      *item = (void *) list->head;

   else
      *item = NULL;

   list->curr = list->head;
   list->prev = NULL;

   return 0;
}


/*
 * lstEnd
 *    
 *
 * Parameters:
 *    list                       a pointer to a lst list
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

int lstEnd(lst_t *list)
{
   while (list->curr) {
      list->prev = list->curr;
      list->curr = (lstListItem_t *) (list->curr->next);
   }

   return 0;
}


/*
 * lstTail
 *    
 *
 * Parameters:
 *    list                       a pointer to a lst list
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

int lstTail(lst_t *list, void **item)
{
   if (!list->curr) {
      list->curr = list->head;
      list->prev = NULL;
   }

   if (!list->curr) {
      *item = NULL;
      return 0;
   }

   while (list->curr->next) {
      list->prev = list->curr;
      list->curr = (lstListItem_t *) (list->curr->next);
   }

   *item = list->curr;
   return 0;
}


/*
 * lstCurr
 *    
 *
 * Parameters:
 *    list                       a pointer to a lst list
 *    item                       used to return a pointer to the current item
 *                               of the list
 *
 * Function:
 *    This returns a pointer to the current item of the list. 
 *    Notice that no copying of contents of the given item is done, 
 *    i.e. only the pointer to the item is used.
 *
 *    If there are no items left in the list, NULL will be returned in
 *    the item parameter.
 *
 * Returns:
 *    >= 0 if the function was successful
 *    < 0  indicating an error
 *
 */

int lstCurr(lst_t *list, void **item)
{
   *item = list->curr;
   return 0;
}


/*
 * lstNext
 *    
 *
 * Parameters:
 *    list                       a pointer to a lst list
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
 *    the item parameter.
 *
 * Returns:
 *    >= 0 if the function was successful
 *    < 0  indicating an error
 *
 */

int lstNext(lst_t *list, void **item)
{
   if (list->curr) {
      list->prev = list->curr;
      list->curr = (lstListItem_t *) (list->curr->next);
      *item = (void *) list->curr;
   }

   else
      *item = NULL;

   return 0;
}


int lstNextExists(lst_t *list)
{
   return (list->curr && list->curr->next) ? 1 : 0;
}

/*
 * lstAdd
 *    
 *
 * Parameters:
 *    list                       a pointer to a lst list
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

int lstAdd(lst_t *list, void *item)
{
   ((lstListItem_t *) item)->next = list->curr;

   if (list->prev)
      list->prev->next = item;

   else
      list->head = (lstListItem_t *) item;

   list->curr = (lstListItem_t *) item;

   list->numItems++;

   return 0;
}


// End of File
