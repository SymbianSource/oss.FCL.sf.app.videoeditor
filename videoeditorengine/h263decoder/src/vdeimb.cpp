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
* Individual image buffer handling.
*
*/


/*
 * Includes
 */


#include "h263dConfig.h"
#include "vdeimb.h"

#include "vde.h"


/*
 * Global functions
 */

/* {{-output"vdeImbAlloc.txt"}} */
/*
 * vdeImbAlloc
 *
 * Parameters:
 *    width                      width of the luminance image in pixels
 *    height                     height of the luminance image in pixels
 *
 * Function:
 *    This function allocates and initializes a new image buffer of
 *    requested size.
 *
 * Returns:
 *    a pointer to the created image buffer,
 *    or NULL if the function failed
 *
 *    
 */

vdeImb_t *vdeImbAlloc(int width, int height, int fYuvNeeded)
/* {{-output"vdeImbAlloc.txt"}} */
{
   vdeImb_t *imb;

   imb = (vdeImb_t *) vdeMalloc(sizeof(vdeImb_t));
   if (imb == NULL)
      return NULL;
   memset(imb, 0, sizeof(vdeImb_t));


   imb->drawItem = renDriAlloc(width, height, fYuvNeeded);
   if (imb->drawItem == NULL)
      goto errRenDriAlloc;
   {
      int numMBsInPicture;
      
      numMBsInPicture = width * height / 256;

      imb->yQuantParams = (int *) vdeMalloc(numMBsInPicture * sizeof(int));
      if (imb->yQuantParams == NULL)
         goto errYQuantParams;

      imb->uvQuantParams = (int *) vdeMalloc(numMBsInPicture * sizeof(int));
      if (imb->uvQuantParams == NULL)
         goto errUVQuantParams;

   }

   return imb;

   /* Error situations: release everything in reverse order */

errUVQuantParams:

      vdeDealloc(imb->yQuantParams);
errYQuantParams:

      renDriFree(imb->drawItem);

errRenDriAlloc:

   vdeDealloc(imb);

   return NULL;
}


/* {{-output"vdeImbCopyParameters.txt"}} */
/*
 * vdeImbCopyParameters
 *
 * Parameters:
 *    dstImb                     destination image buffer
 *    srcImb                     source image buffer
 *
 * Function:
 *    This function copies the srcImb structure to the dstImb structure.
 *    All other parameters are copied but the actual picture contents.
 *    The function handles nested structures correctly.
 *
 * Returns:
 *    (See vde.h for descriptions of return values.)
 *    VDE_OK
 *    VDE_ERROR
 *
 *    
 */

int vdeImbCopyParameters(vdeImb_t *dstImb, const vdeImb_t *srcImb)
/* {{-output"vdeImbCopyParameters.txt"}} */
{
   /* drawItem */
   renDriCopyParameters(dstImb->drawItem, srcImb->drawItem);

   dstImb->fReferenced = srcImb->fReferenced;
   dstImb->tr = srcImb->tr;
   dstImb->trRef = srcImb->trRef;

   {
      int numOfMBs = renDriNumOfMBs(srcImb->drawItem);

      /* yQuantParams */
      memcpy(
         dstImb->yQuantParams, 
         srcImb->yQuantParams, 
         numOfMBs * sizeof(int));
      
      /* uvQuantParams */
      memcpy(
         dstImb->uvQuantParams, 
         srcImb->uvQuantParams, 
         numOfMBs * sizeof(int));

   }

   return VDE_OK;
}


/* {{-output"vdeImbDealloc.txt"}} */
/*
 * vdeImbDealloc
 *
 * Parameters:
 *    imb                        a pointer to image buffer to destroy
 *
 * Function:
 *    This function deallocates the given image buffer.
 *
 * Returns:
 *    Nothing
 *
 *    
 */

void vdeImbDealloc(vdeImb_t *imb)
/* {{-output"vdeImbDealloc.txt"}} */
{
   if (!imb)
      return;

   if (imb->drawItem)
      renDriFree(imb->drawItem);

    vdeDealloc(imb->yQuantParams);
    vdeDealloc(imb->uvQuantParams);

   vdeDealloc(imb);
}
// End of File
