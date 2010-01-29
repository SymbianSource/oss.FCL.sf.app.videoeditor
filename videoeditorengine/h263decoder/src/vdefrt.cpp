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
* Frame type -dependent settings.
*
*/




/*
 * Includes
 */


#include "h263dConfig.h"

#include "vdefrt.h"
#include "vde.h"
#include "h263dapi.h"



/*
 * Defines
 */

/* The following two definitions are used to indicate that all image 
   widths/heights belong to the scope of the correponding setting. */
#define VDEFRT_ALL_WIDTHS 0
#define VDEFRT_ALL_HEIGHTS 0


/*
 * Structures and typedefs
 */

/* Size list item */
typedef struct {
   LST_ITEM_SKELETON

   int width;                    /* image width associated with this item */
   int height;                   /* image height associated with this item */
   u_char fSnapshot;             /* snapshot flag associated with this item */

   u_int32 dataHandle;           /* handle to generic data */
   void (*dealloc) (void *);     /* function for dataHandle dealloction */
} vdeFrtSizeListItem_t;

/* NOTE: Currently the snapshot scope must be used together with the frame 
         size scope. It is not possible to store or retrieve an item based 
         on the snapshot flag only. */

/*
 * Local function prototypes
 */

static int vdeFrtRemoveAll(vdeFrtStore_t *store);

static int vdeFrtSeekSize(lst_t *sizeList, int width, int height, 
   vdeFrtSizeListItem_t **seekHit);

static int vdeFrtSeekItem(lst_t *sizeList, int width, int height, 
   u_char fSnapshot, vdeFrtSizeListItem_t **seekHit);


/*
 * Global functions
 */

/* {{-output"vdeFrtClose.txt"}} */
/*
 * vdeFrtClose
 *
 * Parameters:
 *    store                      a pointer to a frame type store
 *
 * Function:
 *    This function closes the given frame type store.
 *
 * Returns:
 *    VDE_OK                     if the function was successful
 *    VDE_ERROR                  if an error occured
 *
 *    
 */

int vdeFrtClose(vdeFrtStore_t *store)
/* {{-output"vdeFrtClose.txt"}} */
{
   int retValue;

   retValue = vdeFrtRemoveAll(store);
   if (retValue < 0)
      return retValue;

   if (lstClose(&store->sizeList) < 0)
      return VDE_ERROR;

   return VDE_OK;
}


/* {{-output"vdeFrtGetItem.txt"}} */
/*
 * vdeFrtGetItem
 *
 * Parameters:
 *    store                      a pointer to a frame type store
 *    frameType                  specifies the frame type for the wanted item
 *    item                       used to return an item from the store
 *
 * Function:
 *    This function fetches the item from the given store which corresponds
 *    to the given frame type.
 *
 * Returns:
 *    VDE_OK                     if the function was successful
 *    VDE_ERROR                  if an error occured
 *
 *    
 */

int vdeFrtGetItem(vdeFrtStore_t *store, h263dFrameType_t *frameType, u_int32 *item)
/* {{-output"vdeFrtGetItem.txt"}} */
{
   switch (frameType->scope) {
      case H263D_FTYPE_NDEF:
      case H263D_FTYPE_SIZE + H263D_FTYPE_SNAPSHOT:
         {
            int retValue;
            vdeFrtSizeListItem_t *listItem;

            /* seek item with matching size and snapshot flag */
            retValue = vdeFrtSeekItem(&store->sizeList, frameType->width, 
               frameType->height, frameType->fSnapshot, &listItem);
            if (retValue < 0)
               return retValue;

            if (listItem)
               *item = listItem->dataHandle;

            else {
               if (lstTail(&store->sizeList, (void **) &listItem) < 0)
                  return VDE_ERROR;

               /* If a default item exists */
               if (listItem &&
                  listItem->width == VDEFRT_ALL_WIDTHS && 
                  listItem->height == VDEFRT_ALL_HEIGHTS)
                  *item = listItem->dataHandle;

               else {
                  *item = 0;
                  return VDE_OK_NOT_AVAILABLE;
               }
            }

            return VDE_OK;

         }

      case H263D_FTYPE_SIZE:
         {
            int retValue;
            vdeFrtSizeListItem_t *listItem;

            retValue = vdeFrtSeekSize(&store->sizeList, frameType->width, 
               frameType->height, &listItem);
            if (retValue < 0)
               return retValue;

            if (listItem)
               *item = listItem->dataHandle;

            else {
               if (lstTail(&store->sizeList, (void **) &listItem) < 0)
                  return VDE_ERROR;

               /* If a default item exists */
               if (listItem &&
                  listItem->width == VDEFRT_ALL_WIDTHS && 
                  listItem->height == VDEFRT_ALL_HEIGHTS)
                  *item = listItem->dataHandle;

               else {
                  *item = 0;
                  return VDE_OK_NOT_AVAILABLE;
               }
            }

            return VDE_OK;
         }

      case H263D_FTYPE_DEF:
         {
            vdeFrtSizeListItem_t *tailItem;

            if (lstTail(&store->sizeList, (void **) &tailItem) < 0)
               return VDE_ERROR;

            /* If a default item exists */
            if (tailItem->width == VDEFRT_ALL_WIDTHS && 
               tailItem->height == VDEFRT_ALL_HEIGHTS) {
               *item = tailItem->dataHandle;
               return VDE_OK;
            }

            else {
               *item = 0;
               return VDE_OK_NOT_AVAILABLE;
            }
         }

      default:
         return VDE_ERROR;
   }
}


/* {{-output"vdeFrtOpen.txt"}} */
/*
 * vdeFrtOpen
 *
 * Parameters:
 *    store                      a pointer to a frame type store
 *
 * Function:
 *    This function initializes the given frame type store.
 *
 * Returns:
 *    VDE_OK                     if the function was successful
 *    VDE_ERROR                  if an error occured
 *
 *    
 */

int vdeFrtOpen(vdeFrtStore_t *store)
/* {{-output"vdeFrtOpen.txt"}} */
{
   if (lstOpen(&store->sizeList) < 0)
      return VDE_ERROR;

   return VDE_OK;
}


/* {{-output"vdeFrtPutItem.txt"}} */
/*
 * vdeFrtPutItem
 *
 * Parameters:
 *    store                      a pointer to a frame type store
 *    frameType                  specifies the frame type for the given item
 *    item                       a data handle to store
 *    removeItem                 a function which is used to deallocate
 *                               the data handle when it is removed from
 *                               the store. NULL may be given if no
 *                               deallocation is needed.
 *
 * Function:
 *    This function puts a data handle to the frame type store.
 *
 * Returns:
 *    VDE_OK                     if the function was successful
 *    VDE_ERROR                  if an error occured
 *
 *    
 */

int vdeFrtPutItem(vdeFrtStore_t *store, h263dFrameType_t *frameType, 
   u_int32 item, void (*removeItem) (void *))
/* {{-output"vdeFrtPutItem.txt"}} */
{
   switch (frameType->scope) {
      case H263D_FTYPE_ALL:
         {
            int retValue;
            vdeFrtSizeListItem_t *newItem;

            retValue = vdeFrtRemoveAll(store);
            if (retValue < 0)
               return retValue;

            newItem = (vdeFrtSizeListItem_t *) 
               vdeMalloc(sizeof(vdeFrtSizeListItem_t));
            if (!newItem)
               return VDE_ERROR;

            newItem->width = VDEFRT_ALL_WIDTHS;
            newItem->height = VDEFRT_ALL_HEIGHTS;
            newItem->fSnapshot = 0;
            newItem->dataHandle = item;
            newItem->dealloc = removeItem;

            if (lstAdd(&store->sizeList, newItem) < 0) {
               vdeDealloc(newItem);
               return VDE_ERROR;
            }

            return VDE_OK;
         }

      case H263D_FTYPE_DEF:
         {
            vdeFrtSizeListItem_t *tailItem;

            if (lstTail(&store->sizeList, (void **) &tailItem) < 0)
               return VDE_ERROR;

            /* If a default item exists */
            if (tailItem->width == VDEFRT_ALL_WIDTHS && 
               tailItem->height == VDEFRT_ALL_HEIGHTS) {

               if (tailItem->dealloc)
                  tailItem->dealloc((void *) tailItem->dataHandle);

               /* Overwrite its data handle */
               tailItem->dataHandle = item;
            }

            else {
               vdeFrtSizeListItem_t *newItem;

               newItem = (vdeFrtSizeListItem_t *) 
                  vdeMalloc(sizeof(vdeFrtSizeListItem_t));
               if (!newItem)
                  return VDE_ERROR;

               newItem->width = VDEFRT_ALL_WIDTHS;
               newItem->height = VDEFRT_ALL_HEIGHTS;
               newItem->fSnapshot = 0;
               newItem->dataHandle = item;
               newItem->dealloc = removeItem;

               if (lstEnd(&store->sizeList) < 0) {
                  vdeDealloc(newItem);
                  return VDE_ERROR;
               }

               if (lstAdd(&store->sizeList, newItem) < 0) {
                  vdeDealloc(newItem);
                  return VDE_ERROR;
               }
            }

            return VDE_OK;
         }

      case H263D_FTYPE_SIZE:
         {
            int retValue;
            vdeFrtSizeListItem_t *listItem;

            retValue = vdeFrtSeekSize(&store->sizeList, frameType->width, 
               frameType->height, &listItem);
            if (retValue < 0)
               return retValue;

            /* If a setting with the same size already exists */
            if (listItem) {

               if (listItem->dealloc)
                  listItem->dealloc((void *) listItem->dataHandle);
 
               /* Overwrite its data handle */
               listItem->dataHandle = item;
            }

            else {
               vdeFrtSizeListItem_t *newItem;

               if (lstHead(&store->sizeList, (void **) &listItem) < 0)
                  return VDE_ERROR;

               newItem = (vdeFrtSizeListItem_t *) 
                  vdeMalloc(sizeof(vdeFrtSizeListItem_t));
               if (!newItem)
                  return VDE_ERROR;

               newItem->width = frameType->width;
               newItem->height = frameType->height;
               newItem->fSnapshot = 0;
               newItem->dataHandle = item;
               newItem->dealloc = removeItem;

               if (lstAdd(&store->sizeList, newItem) < 0) {
                  vdeDealloc(newItem);
                  return VDE_ERROR;
               }
            }

            return VDE_OK;
         }

      case H263D_FTYPE_SIZE + H263D_FTYPE_SNAPSHOT:
         { 
            int retValue;
            vdeFrtSizeListItem_t *listItem;

            retValue = vdeFrtSeekItem(&store->sizeList, frameType->width, 
               frameType->height, frameType->fSnapshot, &listItem);
            if (retValue < 0)
               return retValue;

            /* If a setting with the same parameters already exists */
            if (listItem) {

               if (listItem->dealloc)
                  listItem->dealloc((void *) listItem->dataHandle);
 
               /* Overwrite its data handle */
               listItem->dataHandle = item;
            }

            else {
               vdeFrtSizeListItem_t *newItem;

               if (lstHead(&store->sizeList, (void **) &listItem) < 0)
                  return VDE_ERROR;

               newItem = (vdeFrtSizeListItem_t *) 
                  vdeMalloc(sizeof(vdeFrtSizeListItem_t));
               if (!newItem)
                  return VDE_ERROR;

               newItem->width = frameType->width;
               newItem->height = frameType->height;
               newItem->fSnapshot = frameType->fSnapshot;
               newItem->dataHandle = item;
               newItem->dealloc = removeItem;

               if (lstAdd(&store->sizeList, newItem) < 0) {
                  vdeDealloc(newItem);
                  return VDE_ERROR;
               }
            }

            return VDE_OK;           

         }
      default:
         return VDE_ERROR;
   }
}


/* {{-output"vdeFrtRemoveItem.txt"}} */
/*
 * vdeFrtRemoveItem
 *
 * Parameters:
 *    store                      a pointer to a frame type store
 *    frameType                  specifies the frame type for the item to remove
 *
 * Function:
 *    This function removes an entry from the given frame type store.
 *    The entry corresponds to the given frame type.
 *
 * Returns:
 *    VDE_OK                     if the function was successful
 *    VDE_ERROR                  if an error occured
 *
 *    
 */

int vdeFrtRemoveItem(vdeFrtStore_t *store, h263dFrameType_t *frameType)
/* {{-output"vdeFrtRemoveItem.txt"}} */
{
   switch (frameType->scope) {
      case H263D_FTYPE_ALL:
         {
            int retValue;

            retValue = vdeFrtRemoveAll(store);
            if (retValue < 0)
               return retValue;

            return VDE_OK;
         }

      case H263D_FTYPE_DEF:
         {
            vdeFrtSizeListItem_t *tailItem;

            if (lstTail(&store->sizeList, (void **) &tailItem) < 0)
               return VDE_ERROR;

            /* If a default item exists */
            if (tailItem && tailItem->width == VDEFRT_ALL_WIDTHS && 
               tailItem->height == VDEFRT_ALL_HEIGHTS) {

               /* Remove it */
               if (lstRemove(&store->sizeList, (void **) &tailItem) < 0)
                  return VDE_ERROR;

               if (tailItem->dealloc)
                  tailItem->dealloc((void *) tailItem->dataHandle);
                  
               vdeDealloc(tailItem); 
            }

            return VDE_OK;
         }

      case H263D_FTYPE_SIZE:
         {
            int retValue;
            vdeFrtSizeListItem_t *listItem;

            retValue = vdeFrtSeekSize(&store->sizeList, frameType->width, 
               frameType->height, &listItem);
            if (retValue < 0)
               return retValue;

            /* If an item with the same size already exists */
            if (listItem) {

               /* Remove it */
               if (lstRemove(&store->sizeList, (void **) &listItem) < 0)
                  return VDE_ERROR;

               if (listItem->dealloc)
                  listItem->dealloc((void *) listItem->dataHandle);
                  
               vdeDealloc(listItem); 
            }

            return VDE_OK;
         }

      case H263D_FTYPE_SIZE + H263D_FTYPE_SNAPSHOT:
         {
            int retValue;
            vdeFrtSizeListItem_t *listItem;

            retValue = vdeFrtSeekItem(&store->sizeList, frameType->width, 
               frameType->height, frameType->fSnapshot, &listItem);
            if (retValue < 0)
               return retValue;

            /* If an item with the same parameters already exists */
            if (listItem) {

               /* Remove it */
               if (lstRemove(&store->sizeList, (void **) &listItem) < 0)
                  return VDE_ERROR;

               if (listItem->dealloc)
                  listItem->dealloc((void *) listItem->dataHandle);
                  
               vdeDealloc(listItem); 
            }
            return VDE_OK;

         }

      default:
         return VDE_ERROR;
   }
}


/*
 * Local functions
 */

/*
 * vdeFrtRemoveAll
 *    This function removes all items from the given store.
 */

static int vdeFrtRemoveAll(vdeFrtStore_t *store)
{
   vdeFrtSizeListItem_t *listItem;

   if (lstHead(&store->sizeList, (void **) &listItem) < 0)
      return VDE_ERROR;

   do {
      if (lstRemove(&store->sizeList, (void **) &listItem) < 0)
         return VDE_ERROR;

      if (listItem) {
         if (listItem->dealloc)
            listItem->dealloc((void *) listItem->dataHandle);      
         vdeDealloc(listItem);
      }
   } while (listItem);

   return VDE_OK;
}


/*
 * vdeFrtSeekSize
 *    This function seeks for the given width and height from the given 
 *    size list. It returns the matching size list item (if there is one).
 */

static int vdeFrtSeekSize(lst_t *sizeList, int width, int height, 
   vdeFrtSizeListItem_t **seekHit)
{
   vdeFrtSizeListItem_t *listItem;

   *seekHit = NULL;

   if (lstHead(sizeList, (void **) &listItem) < 0)
      return VDE_ERROR;

   while (listItem) {
      if (lstNext(sizeList, (void **) &listItem) < 0)
         return VDE_ERROR;

      if (listItem && listItem->width == width && listItem->height == height) {
         *seekHit = listItem;
         break;
      }
   }

   return VDE_OK;
}

/*
 * vdeFrtSeekItem
 *    This function seeks for an item with the given parameters from the 
 *    size list. It returns the matching list item (if there is one).
 */

static int vdeFrtSeekItem(lst_t *sizeList, int width, int height, 
   u_char fSnapshot, vdeFrtSizeListItem_t **seekHit)
{
   vdeFrtSizeListItem_t *listItem;

   *seekHit = NULL;

   if (lstHead(sizeList, (void **) &listItem) < 0)
      return VDE_ERROR;

   while (listItem) {

      if (listItem && listItem->width == width && listItem->height == height &&
          listItem->fSnapshot == fSnapshot) {
         *seekHit = listItem;
         break;
      }
      if (lstNext(sizeList, (void **) &listItem) < 0)
        return VDE_ERROR;
   }

   return VDE_OK;
}

// End of File
