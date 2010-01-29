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
* Frame memory organization module.
*
*/




/*
 * Includes
 */


#include "h263dConfig.h"
#include "vdeims.h"

#include "debug.h"
#include "vde.h"
#include "vdeimb.h"


/*
 * Structs and typedefs
 */

/* Structure for a single "Free" store */
typedef struct {
   LST_ITEM_SKELETON

   lifo_t freeStore;    /* a stack of free image store items */
   int lumWidth;        /* width of the items in freeStore */
   int lumHeight;       /* height of the items in freeStore
                           (in luminance pixels) */
   int fYuvNeeded;                            
} vdeImsFreeStore_t;


/*
 * Local function prototypes
 */

static vdeImsItem_t *vdeImsAllocItem(int lumWidth, int lumHeight, int fYuvNeeded);

static vdeImsFreeStore_t *vdeImsAllocFreeStore(int numFreeItems, 
   int lumWidth, int lumHeight, int fYuvNeeded);

static int vdeImsDeallocFreeStore(vdeImsFreeStore_t *store);

static int vdeImsGetFreeStore(lst_t *freeStoreList, int lumWidth, int lumHeight,
   vdeImsFreeStore_t **freeStore);


/*
 * Global functions
 */

/* {{-output"vdeImsChangeReference.txt"}} */
/*
 * vdeImsChangeReference
 *    
 *
 * Parameters:
 *    store                      a pointer to an image store
 *    newItem                    new reference frame to replace a corresponding
 *                               reference frame in the image store
 *
 * Function:
 *    This function is used to change a reference frame in the image store.
 *    The function is passed a new reference frame. Then, the function
 *    searches for a reference frame with the same TR as the passed frame
 *    (from the image store). If such a frame is found, the function
 *    takes the old referece frame away from the reference store and puts
 *    it to the free or idle store depending on whether the frame has already
 *    been displayed or not. The new reference frame is placed into 
 *    the reference store. If a corresponding reference frame is not found
 *    from the image store, the function returns VDE_OK_NOT_AVAILABLE.
 *
 *    This function is intended to be used together with error concealment
 *    methods capable of repairing past frames.
 *
 * Returns:
 *    VDE_OK                     if the function was successful
 *    VDE_OK_NOT_AVAILABLE       the function behaved as expected but
 *                               a requested frame was not found
 *    VDE_ERROR                  if an error occured
 *
 *    
 */

int vdeImsChangeReference(vdeIms_t *store, vdeImsItem_t *newItem)
/* {{-output"vdeImsChangeReference.txt"}} */
{
   vdeImsItem_t *oldItem; /* old reference frame */

    /* Note: This case may not be ever used, but it is supported for 
     consistency */

    /* If the TRs of the reference frames do not match */
    if (store->refFrame->imb->tr == newItem->imb->tr)
     /* Return with not found indication */
     return VDE_OK_NOT_AVAILABLE;

    /* Store the new reference to the image store */
    oldItem = store->refFrame;
    store->refFrame = newItem;

    /* Add the old reference to the "Free" store */
    if (vdeImsPutFree(store, oldItem) < 0)
         return VDE_ERROR;

   return VDE_OK;
}


/* {{-output"vdeImsClose.txt"}} */
/*
 * vdeImsClose
 *    
 *
 * Parameters:
 *    store                      a pointer to an image store
 *
 * Function:
 *    This function closes the given image store.
 *    Notice that the memory pointed by the given image store pointer is not
 *    deallocated but rather it is assumed that this memory is either static
 *    or externally deallocated.
 *
 * Returns:
 *    VDE_OK                     if the function was successful
 *    VDE_ERROR                  if an error occured
 *
 *    
 */

int vdeImsClose(vdeIms_t *store)
/* {{-output"vdeImsClose.txt"}} */
{
   vdeImb_t *imb;
   vdeImsFreeStore_t *freeStore;
   vdeImsItem_t *pItem;

   /* freeStoreList */
   if (lstHead(&store->freeStoreList, (void **) &freeStore) < 0)
      return VDE_ERROR;

   do {
      if (lstRemove(&store->freeStoreList, (void **) &freeStore) < 0)
         return VDE_ERROR;
      
      if (freeStore) {
         if (vdeImsDeallocFreeStore(freeStore) < 0)
            return VDE_ERROR;
      }
   } while (freeStore);

   if (lstClose(&store->freeStoreList) < 0)
      return VDE_ERROR;

   if (store->refFrame) {
     vdeImbDealloc(store->refFrame->imb);
     vdeDealloc(store->refFrame);
   }

   /* idleStore */
   if (lstHead(&store->idleStore, (void **) &pItem) < 0)
      return VDE_ERROR;

   do {
      if (lstRemove(&store->idleStore, (void **) &pItem) < 0)
         return VDE_ERROR;

      if (pItem) {
         imb = pItem->imb;
         vdeDealloc(pItem);
         vdeImbDealloc(imb);
      }
   } while (pItem);

   if (lstClose(&store->idleStore) < 0)
      return VDE_ERROR;

   return VDE_OK;
}



/* {{-output"vdeImsGetFree.txt"}} */
/*
 * vdeImsGetFree
 *    
 *
 * Parameters:
 *    store                      a pointer to an image store
 *    lumWidth                   width of the luminance image (in pixels)
 *    lumHeight                  height of the luminance image (in pixels)
 *    item                       a free image store item
 *
 * Function:
 *    This function is used to get a free frame memory (of requested size).
 *    If a free frame memory (image store item) does not exist, a new one
 *    is allocated and initialized.
 *
 * Returns:
 *    VDE_OK                     if the function was successful
 *    VDE_ERROR                  if an error occured
 *
 *    
 */

int vdeImsGetFree(vdeIms_t *store, int lumWidth, int lumHeight, 
   vdeImsItem_t **item)
/* {{-output"vdeImsGetFree.txt"}} */
{
   vdeImsFreeStore_t *freeStore, *tmpFreeStore;
   vdeImsItem_t *pItem;

   if (vdeImsGetFreeStore(&store->freeStoreList, lumWidth, lumHeight, 
      &freeStore) < 0)
      return VDE_ERROR;

   if (freeStore) {
      if (lifoGet(&freeStore->freeStore, (void **) &pItem) < 0)
         return VDE_ERROR;
      if (pItem) {
         *item = pItem;
         return VDE_OK;
      }
      else {
         *item = vdeImsAllocItem(lumWidth, lumHeight, store->fYuvNeeded);
         return VDE_OK;
      }
   }

   else {
      freeStore = vdeImsAllocFreeStore(0, lumWidth, lumHeight, store->fYuvNeeded);
      if (freeStore == NULL)
         return VDE_ERROR;

      if (lstHead(&store->freeStoreList, (void **) &tmpFreeStore) < 0) {
         vdeImsDeallocFreeStore(freeStore);
         return VDE_ERROR;
      }

      if (lstAdd(&store->freeStoreList, freeStore) < 0) {
         vdeImsDeallocFreeStore(freeStore);
         return VDE_ERROR;
      }

      *item = vdeImsAllocItem(lumWidth, lumHeight, store->fYuvNeeded);
      return VDE_OK;
   }
}


/* {{-output"vdeImsGetReference.txt"}} */
/*
 * vdeImsGetReference
 *    
 *
 * Parameters:
 *    store                      a pointer to an image store
 *    mode                       VDEIMS_REF_LATEST = latest reference frame
 *                               VDEIMS_REF_OLDEST = oldest reference frame
 *                               VDEIMS_REF_TR = frame specified by tr,
 *                                  see also the explation for
 *                                  VDEIMS_GET_CLOSEST_REFERENCE
 *                                  in the module description at the beginning
 *                                  of this file
 *    tr                         if mode is equal to VDEIMS_REF_TR,
 *                               this parameter specifies the TR (temporal
 *                               reference) of the reference frame
 *    item                       used to return the specified reference frame
 *
 * Function:
 *    This function is used to get a reference frame memory (image store item).
 *    One can either query for the latest reference frame or for a frame
 *    having a certain temporal reference value.
 *
 * Returns:
 *    VDE_OK                     if the function was successful
 *    VDE_ERROR                  if an error occured
 *
 *    
 */

int vdeImsGetReference(vdeIms_t *store, int mode, int tr, vdeImsItem_t **item)
/* {{-output"vdeImsGetReference.txt"}} */
{
   vdeAssert(
      mode == VDEIMS_REF_LATEST || 
      mode == VDEIMS_REF_OLDEST || 
      mode == VDEIMS_REF_TR);

   switch (mode) {
      case VDEIMS_REF_LATEST:
         *item = store->refFrame;
         break;

      case VDEIMS_REF_OLDEST:
         *item = store->refFrame;
         break;

      case VDEIMS_REF_TR:
         /* This section of code returns the reference frame having the same
            TR as requested. If there is no such TR, the code returns a NULL
            frame. */
         if (store->refFrame->imb->tr == tr)
             *item = store->refFrame;
         else
             *item = NULL;
         break;
   }        

   return VDE_OK;
}


/* {{-output"vdeImsOpen.txt"}} */
/*
 * vdeImsOpen
 *    
 *
 * Parameters:
 *    store                      a pointer to an image store
 *    numFreeItems               number of initial free image store items
 *    lumWidth                   width of the luminance image (in pixels)
 *                               for free items
 *    lumHeight                  height of the luminance image (in pixels)
 *                               for free items
 *
 * Function:
 *    This function initializes the given image store. The function allocates
 *    and initializes a given number of free image store items of given size.
 *    The Reference image store is initialized to carry multiple image store
 *    items, if the Reference Picture Selection mode is indicated. Otherwise,
 *    the Reference image store carries only one image store item at a time.
 *
 * Returns:
 *    VDE_OK                     if the function was successful
 *    VDE_ERROR                  if an error occured
 *
 *    
 */

int vdeImsOpen(vdeIms_t *store, int /*numFreeItems*/, int /*lumWidth*/, int /*lumHeight*/)
{
   vdeImsFreeStore_t *freeStore;

   store->refFrame = 0;

   if (lstOpen(&store->idleStore) < 0)
      goto errIdleStore;
   
// not ready to open freeStore yet
    freeStore = NULL;

   if (lstOpen(&store->freeStoreList) < 0)
      goto errOpenFreeStoreList;


   return VDE_OK;

   /* Error cases */
   errOpenFreeStoreList:

   vdeImsDeallocFreeStore(freeStore);

   lstClose(&store->idleStore);
   errIdleStore:

   return VDE_ERROR;
}

void vdeImsSetYUVNeed(vdeIms_t *store, int fYuvNeeded)
{
    store->fYuvNeeded = fYuvNeeded;
}
    
/* {{-output"vdeImsPut.txt"}} */
/*
 * vdeImsPut
 *    
 *
 * Parameters:
 *    store                      a pointer to an image store
 *    item                       image store item to put into the image store
 *
 * Function:
 *    This function is used to return a filled image store item into
 *    the image store. If the item is possibly referenced later on, it is
 *    stored into the Reference store. If the item is not referenced but
 *    it is going to be displayed, it is put into the Idle store.
 *    Otherwise, the item is put into the Free store. 
 *
 * Returns:
 *    VDE_OK                     if the function was successful
 *    VDE_ERROR                  if an error occured
 *
 *    
 */

int vdeImsPut(vdeIms_t *store, vdeImsItem_t *item)
/* {{-output"vdeImsPut.txt"}} */
{
   vdeImb_t *imb;

   imb = item->imb;

   /* If the item is referenced */
   if (imb->fReferenced) {

     /* If an old reference exists */
     if (store->refFrame) {
       /* Add the old reference to the "Free" store */
       if (vdeImsPutFree(store, store->refFrame) < 0)
          return VDE_ERROR;
     }

     store->refFrame = item;
   }

   /* Else (the item is not referenced) */
   else {
     /* Add the item to the "Free" store */
     if (vdeImsPutFree(store, item) < 0)
        return VDE_ERROR;
   }

   return VDE_OK;
}


/* {{-output"vdeImsPutFree.txt"}} */
/*
 * vdeImsPutFree
 *    
 *
 * Parameters:
 *    store                      a pointer to an image store
 *    item                       image store item to put into the Free store
 *
 * Function:
 *    This function puts an image store item to a suitable Free store
 *    of the passed image store. It selects the Free store according to 
 *    the image size or creates a new Free store if a suitable Free store 
 *    does not already exist.
 *
 *    One can use this function instead of vdeImsPut to explicitly discard
 *    an image store item (whereas for vdeImsPut, one would have to set
 *    the correct flags and pass a NULL renderer handle to get the same
 *    effect).
 *
 * Returns:
 *    VDE_OK                     if the function was successful
 *    VDE_ERROR                  if an error occured
 *
 *    
 */

int vdeImsPutFree(vdeIms_t *store, vdeImsItem_t *item)
/* {{-output"vdeImsPutFree.txt"}} */
{
   vdeImsFreeStore_t *freeStore, *tmpFreeStore;
   int 
      lumWidth = renDriBitmapWidth(item->imb->drawItem),
      lumHeight = renDriBitmapHeight(item->imb->drawItem);

   if (vdeImsGetFreeStore(&store->freeStoreList, lumWidth, 
      lumHeight, &freeStore) < 0)
      return VDE_ERROR;

   if (freeStore == NULL) {
      /* The same algorithm as in vdeImsGetFree */
      freeStore = vdeImsAllocFreeStore(0, lumWidth, lumHeight, store->fYuvNeeded);
      if (freeStore == NULL)
         return VDE_ERROR;

      if (lstHead(&store->freeStoreList, (void **) &tmpFreeStore) < 0) {
         vdeImsDeallocFreeStore(freeStore);
         return VDE_ERROR;
      }

      if (lstAdd(&store->freeStoreList, freeStore) < 0) {
         vdeImsDeallocFreeStore(freeStore);
         return VDE_ERROR;
      }
   }

   if (lifoPut(&freeStore->freeStore, item) < 0)
      return VDE_ERROR;

   return VDE_OK;
}




/*
 * Local functions
 */

/*
 * vdeImsAllocItem
 *    This function allocates and initializes a new image store item
 *    of given size.
 */

static vdeImsItem_t *vdeImsAllocItem(int lumWidth, int lumHeight, int fYuvNeeded)
{
   vdeImb_t *imb;
   vdeImsItem_t *pItem;

   imb = vdeImbAlloc(lumWidth, lumHeight, fYuvNeeded);
   if (imb == NULL)
      return NULL;

   pItem = (vdeImsItem_t *) vdeMalloc(sizeof(vdeImsItem_t));
   if (pItem == NULL) {
      vdeImbDealloc(imb);
      return NULL;
   }

   pItem->imb = imb;

   return pItem;
}


/*
 * vdeImsAllocFreeStore
 *    This function allocates and initializes a new Free store.
 *    In addition, it allocates a given number of free image store items
 *    of given size and puts these items into the created Free store.
 */

static vdeImsFreeStore_t *vdeImsAllocFreeStore(int numFreeItems, 
   int lumWidth, int lumHeight, int fYuvNeeded)
{
   int i;
   vdeImsFreeStore_t *store;
   vdeImsItem_t *pItem;

   store = (vdeImsFreeStore_t *) vdeMalloc(sizeof(vdeImsFreeStore_t));
   if (store == NULL)
      return NULL;

   store->lumWidth = lumWidth;
   store->lumHeight = lumHeight;
   store->fYuvNeeded = fYuvNeeded;

   if (lifoOpen(&store->freeStore) < 0) {
      vdeDealloc(store);
      return NULL;
   }

   for (i = 0; i < numFreeItems; i++) {
      pItem = vdeImsAllocItem(lumWidth, lumHeight, fYuvNeeded);
      if (pItem == NULL)
         goto error;

      if (lifoPut(&store->freeStore, pItem) < 0)
         goto error;
   }

   return store;

   error:
   if (pItem)
      vdeDealloc(pItem);
   vdeImsDeallocFreeStore(store);
   return NULL;
}


/*
 * vdeImsDeallocFreeStore
 *    This function deallocates a given Free store and all image store items
 *    which were in the store.
 */

static int vdeImsDeallocFreeStore(vdeImsFreeStore_t *store)
{
   vdeImb_t *imb;
   vdeImsItem_t *pItem;

   do {
      if (lifoGet(&store->freeStore, (void **) &pItem) < 0)
         return VDE_ERROR;
      
      if (pItem) {
         imb = pItem->imb;
         vdeDealloc(pItem);
         vdeImbDealloc(imb);
      }
   } while (pItem);

   if (lifoClose(&store->freeStore) < 0)
      return VDE_ERROR;

   vdeDealloc(store);

   return VDE_OK;
}




/*
 * vdeImsGetFreeStore
 *    This function returns a pointer to a Free store which contains frames of
 *    requested size. The Free store is searched from a list of Free stores.
 */

static int vdeImsGetFreeStore(lst_t *freeStoreList, int lumWidth, int lumHeight,
   vdeImsFreeStore_t **freeStore)
{
   if (lstHead(freeStoreList, (void **) freeStore) < 0)
      return VDE_ERROR;

   while (*freeStore && 
      ((*freeStore)->lumWidth != lumWidth || (*freeStore)->lumHeight != lumHeight)) {
      if (lstNext(freeStoreList, (void **) freeStore) < 0)
         return VDE_ERROR;
   }

   return VDE_OK;
}
// End of File
