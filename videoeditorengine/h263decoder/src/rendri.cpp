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
* Renderer Draw Item Interface.
*
*/



#include "h263dconfig.h"
#include "rendri.h"
#include "debug.h"


/*
 * Preprocessor definitions
 */


#define renDriMalloc malloc
#define renDriCalloc calloc
#define renDriRealloc realloc
#define renDriDealloc free

#ifndef renAssert
   #ifndef assert
      #include <assert.h>
   #endif
   #define renAssert(exp) assert(exp)
#endif


/*
 * Local function prototypes
 */


/*
 * Global functions
 */

/* {{-output"renDriAlloc.txt"}} */
/*
 * renDriAlloc
 *    
 *
 * Parameters:
 *    width                      luminance image width in pixels
 *    height                     luminance image height in pixels
 *
 * Function:
 *    This function creates and initializes a new draw item.
 *
 *    When passed to the renderer the following members of the draw item
 *    must be explicitly set (i.e. these members are not set by this 
 *    function):
 *       drawItem->param.dwFlags
 *       drawItem->param.lTime
 *       drawItem->extParam.flags
 *       drawItem->extParam.rate
 *       drawItem->extParam.scale
 *       drawItem->extParam.numOfCodedMBs
 *       drawItem->extParam.snapshotRect
 *       drawItem->extParam.ckInfo
 *       drawItem->retFrame
 *       drawItem->retFrameParam
 *
 * Returns:
 *    a pointer to the created 
 *    draw item                  if the function was successful
 *    NULL                       indicating a general error
 *
 *    
 */

renDrawItem_t * renDriAlloc(int width, int height, int fYuvNeeded)
/* {{-output"renDriAlloc.txt"}} */
{
   renDrawItem_t *drawItem;
   renBitmapInfoHeader_t *bmi;
   void *data;

   /* Allocate draw item */
   drawItem = (renDrawItem_t *) renDriMalloc(sizeof(renDrawItem_t));
   if (drawItem == NULL)
      return NULL;
   memset(drawItem, 0, sizeof(renDrawItem_t));

   /* Allocate bitmap info header */
   bmi = (renBitmapInfoHeader_t *) renDriMalloc(sizeof(renBitmapInfoHeader_t));
   if (bmi == NULL) {
      renDriDealloc(drawItem);
      return NULL;
   }

   /* Initialize bitmap info header */
   bmi->biSize = sizeof(renBitmapInfoHeader_t);
   bmi->biWidth = width;
   bmi->biHeight = height;
   if ( fYuvNeeded )
    {
       bmi->biSizeImage = (u_int32) width * (u_int32) height * 3 / 2;
       /* Allocate room for frame data */
       data = renDriMalloc(bmi->biSizeImage);
       if (data == NULL) {   
          renDriDealloc(bmi);
          renDriDealloc(drawItem);
          return NULL;
       }
    }
   else
    {
        bmi->biSizeImage = 0;
        data = NULL;
    }


   /* Initialize renDrawParam_t member of draw item */
   /* dwFlags set by application */
   drawItem->param.lpFormat = bmi;
   drawItem->param.lpData = data;
   drawItem->param.cbData = bmi->biSizeImage;
   /* lTime set by application */

   drawItem->extParam.size = sizeof(renExtDrawParam_t);
   /* flags set by application */
   /* rate set by application */
   /* scale set by application */
   drawItem->extParam.numOfMBs = (width / 16) * (height / 16);
   /* numOfCodedMBs set by application */
   drawItem->extParam.fCodedMBs = (u_char *) renDriMalloc(
      drawItem->extParam.numOfMBs * sizeof(u_char));
   if (drawItem->extParam.fCodedMBs == NULL) {
      renDriDealloc(data);
      renDriDealloc(bmi);
      renDriDealloc(drawItem);
      return NULL;
   }
   /* snapshotRect set by application */
   /* ckInfo set by application */

   /* retFrame and retFrameParam members of draw item are set by application */

   return drawItem;
}


/* {{-output"renDriCopyParameters.txt"}} */
/*
 * renDriCopyParameters
 *    
 *
 * Parameters:
 *    dstDrawItem                destination draw item
 *    srcDrawItem                source draw item
 *
 * Function:
 *    This function copies the srcDrawItem structure to the dstDrawItem 
 *    structure. All other parameters are copied but the actual picture 
 *    contents. The function handles nested structures correctly.
 *    No pointers are overwritten but rather the contents corresponding
 *    to a pointer are copied from the source to the destionation structure.
 *
 * Returns:
 *    Nothing
 *
 *    
 */

void renDriCopyParameters(
   renDrawItem_t *dstDrawItem, 
   const renDrawItem_t *srcDrawItem)
/* {{-output"renDriCopyParameters.txt"}} */
{
   /* param */
   {
      renDrawParam_t *dstDrawParam = &(dstDrawItem->param);
      const renDrawParam_t *srcDrawParam = &(srcDrawItem->param);

      /* dwFlags */
      dstDrawParam->dwFlags = srcDrawParam->dwFlags;

      /* lpFormat */
      {
         const renBitmapInfoHeader_t *srcBitmapInfoHeader = 
            (renBitmapInfoHeader_t *) srcDrawParam->lpFormat;

         /* biSize indicates the size of the bitmap info header.
            Thus, copy biSize bytes from source to destination bitmap info
            header. 
            Note: it is assumed that biSize (and the amount of allocated
            memory) is the same in both structures. */
         MEMCPY(
            dstDrawParam->lpFormat,
            srcDrawParam->lpFormat,
            (TInt)srcBitmapInfoHeader->biSize);
      }

      /* lpData */
      /* Not copied since contains a pointer to actual picture contents.
         Set to NULL for clarity. */

      /* cbData */
      dstDrawParam->cbData = srcDrawParam->cbData;

      /* lTime */
      dstDrawParam->lTime = srcDrawParam->lTime;
   }

   /* extParam */
   {
      renExtDrawParam_t *dstExtDrawParam = &(dstDrawItem->extParam);
      const renExtDrawParam_t *srcExtDrawParam = &(srcDrawItem->extParam);
      u_char *dstFCodedMBs = dstExtDrawParam->fCodedMBs;

      /* fCodedMBs is the only pointer in the structure. Thus, it is easier
         to temporally save the destionation fCodedMBs, then overwrite each
         member of the destination structure by corresponding member of
         the source structure and finally restore fCodedMBs in the destination
         structure. */

      /* "size" member indicates the size of the structure.
         Thus, copy size bytes from source to destination structure.
         Note: it is assumed that size (and the amount of allocated
         memory) is the same in both structures. */
      MEMCPY(
         (void *) dstExtDrawParam,
         (void *) srcExtDrawParam,
         (TInt)srcExtDrawParam->size);

      /* Restore destination fCodedMBs. */
      dstExtDrawParam->fCodedMBs = dstFCodedMBs;

      /* Copy coded MBs array */
      MEMCPY(
         (void *) dstFCodedMBs,
         (void *) srcExtDrawParam->fCodedMBs,
         (TInt)srcExtDrawParam->numOfMBs);
   }

}

/* {{-output"renDriCopyFrameData.txt"}} */
/*
 * renDriCopyFrameData
 *
 * Parameters:
 *    dstDrawItem                destination draw item
 *    srcDrawItem                source draw item
 *
 * Function:
 *    This function copies the actual picture data contents from srcDrawItem 
 *    structure to the dstDrawItem structure. No pointers are overwritten 
 *    but rather the contents corresponding to a pointer are copied from 
 *    the source to the destination structure.
 *
 * Returns:
 *    Nothing
 *
 */

void renDriCopyFrameData(
   renDrawItem_t *dstDrawItem, 
   const renDrawItem_t *srcDrawItem) 
/* {{-output"renDriCopyFrameData.txt"}} */
{

   renDrawParam_t *dstDrawParam = &(dstDrawItem->param);
   const renDrawParam_t *srcDrawParam = &(srcDrawItem->param);
   const renBitmapInfoHeader_t *srcBitmapInfoHeader = 
      (renBitmapInfoHeader_t *) srcDrawParam->lpFormat;

   /* biSizeImage indicates the size of the image data in bytes.
      Copy biSizeImage bytes from source to destination frame data.
      Note: it is assumed that size (and the amount of allocated
            memory) is the same in both structures. */

   renAssert(((renBitmapInfoHeader_t *) dstDrawParam->lpFormat)->biSizeImage == srcBitmapInfoHeader->biSizeImage);

   MEMCPY(dstDrawParam->lpData,
          srcDrawParam->lpData,
          (TInt)srcBitmapInfoHeader->biSizeImage);  

}

/* {{-output"renDriFree.txt"}} */
/*
 * renDriFree
 *    
 *
 * Parameters:
 *    drawItem                   a pointer to the draw item to free
 *
 * Function:
 *    This function destroys the passed draw item.
 *
 * Returns:
 *    Nothing.
 *
 *    
 */

void renDriFree(renDrawItem_t *drawItem)
/* {{-output"renDriFree.txt"}} */
{
   renDrawParam_t *param;
   renExtDrawParam_t *extParam;

   if (!drawItem)
      return;

   param = &drawItem->param;
   extParam = &drawItem->extParam;

   if (param) {
      if (param->lpFormat)
         renDriDealloc(param->lpFormat);
      if (param->lpData)
         renDriDealloc(param->lpData);
   }

   if (extParam) {
      if (extParam->fCodedMBs)
         renDriDealloc(extParam->fCodedMBs);
   }

   renDriDealloc(drawItem);
}





/* {{-output"renDriYUV.txt"}} */
/*
 * renDriYUV
 *    
 *
 * Parameters:
 *    drawItem                   a pointer to a draw item
 *    y, u, v                    top-left corners of the Y, U and V frames
 *                               corresponding to the passed draw item
 *    width, height              the dimensions of the luminance frame of 
 *                               the passed draw item
 *
 * Function:
 *    This function returns the Y, U and V pointers to the passed draw item
 *    as well as the dimensions of the luminance frame of the draw item.
 *
 * Returns:
 *    REN_OK                     if the function was successful
 *    REN_ERROR                  indicating a general error
 *
 */

int renDriYUV(renDrawItem_t *drawItem,
   u_char **y, u_char **u, u_char **v, int *width, int *height)
/* {{-output"renDriYUV.txt"}} */
{
   renBitmapInfoHeader_t *bmi;
   int32 ySize, uvSize;
   
   /* If invalid arguments */
   if (!drawItem || !drawItem->param.lpFormat || 
      !y || !u || !v || !width || !height) {
      /* Return error */
      debPrintf("renGetYUVFromDrawParam: ERROR - null pointer.\n");
      return REN_ERROR;
   }


   bmi = (renBitmapInfoHeader_t *) drawItem->param.lpFormat;

   *width = bmi->biWidth;
   *height = labs(bmi->biHeight);

   if ( drawItem->param.lpData == NULL )
    {
        // no YUV buffer, parsing-only instance
        *y = NULL,
        *u = NULL;
        *v = NULL;
        
        return REN_OK;
    }

   ySize = (*width) * (*height) * sizeof(u_char);
   uvSize = ySize / 4;

   *y = (u_char *) drawItem->param.lpData;
   *u = *y + ySize;
   *v = *u + uvSize;

   return REN_OK;
}

// End of File
