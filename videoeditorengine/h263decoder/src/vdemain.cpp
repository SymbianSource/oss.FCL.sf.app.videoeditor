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
* Video decoder engine main module.
*
*/




/*
 * Includes
 */

#include "h263dConfig.h"

#include "vdemain.h"
#include "renapi.h"
#include "rendri.h"
#include "h263dext.h"
#include "vde.h"

#include "biblin.h"


/*
 * Global functions
 */
  
/* {{-output"vdeFree.txt"}} */
/*
 * vdeFree
 *    
 *
 * Parameters:
 *    Nothing
 *
 * Function:
 *    This function deinitializes this module.
 *
 * Returns:
 *    VDE_OK                     if the function was successful
 *    VDE_ERROR                  indicating a general error
 *
 *    
 */

int vdeFree(void)
/* {{-output"vdeFree.txt"}} */
{
   if (vdcFree() < 0)
      return VDE_ERROR;

   return VDE_OK;
}


/* {{-output"vdeGetLatestFrame.txt"}} */
/*
 * vdeGetLatestFrame
 *    
 *
 * Parameters:
 *    hInstance                  instance data
 *    
 *    ppy, ppu, ppv              used to return Y, U and V frame pointers
 *
 *    pLumWidth, pLumHeight      used to return luminance image width and height
 *                               Note these values can be counted on even if
 *                               the function returns an error.
 *
 *    pFrameNum                  used to return frame number
 *
 * Function:
 *    This function returns the latest correctly decoded frame
 *    (and some side-information).
 *
 *    Note that no thread synchronization is used since the function
 *    is used only from the mobidntc.c and it is very likely that
 *    the same decoder instance is used only from a single calling
 *    thread at the same time.
 *
 * Returns:
 *    VDE_OK                     if the function was successful
 *    VDE_ERROR                  indicating a general error
 */


int vdeGetLatestFrame(
   vdeHInstance_t hInstance,
   u_char **ppy, u_char **ppu, u_char **ppv,
   int *pLumWidth, int *pLumHeight,
   int *pFrameNum)
/* {{-output"vdeGetLatestFrame.txt"}} */
{
   int
      retValue;

   vdeImb_t
      *pImb;

   vdeImsItem_t 
      *pImsItem;

   vdeInstance_t 
      *instance = (vdeInstance_t *) hInstance;


   vdeAssert(instance != NULL);

   /* Ensure that image width and height are returned anyway */
   *pLumWidth = instance->lumWidth;
   *pLumHeight = instance->lumHeight;

   retValue = vdeImsGetReference(
      &instance->imageStore, 
      VDEIMS_REF_LATEST, 0, 
      &pImsItem);
   if (retValue < 0)
      return retValue;

   if (!pImsItem)
      return VDE_ERROR;

   pImb = pImsItem->imb;

   if (vdeImbYUV(pImb, ppy, ppu, ppv, pLumWidth, pLumHeight) < 0)
      return VDE_ERROR;

   *pFrameNum = renDriFrameNumber(pImb->drawItem);

   return VDE_OK;
}



/* {{-output"vdeInit.txt"}} */
/*
 * vdeInit
 *    
 *
 * Parameters:
 *    param                      a h263dOpen_t structure containing
 *                               the initialization parameters
 *
 * Function:
 *    This function allocates and initializes an H.263 video decoder engine
 *    (VDE) instance.
 *
 * Returns:
 *    a handle to the new instance or
 *    NULL if the initialization fails
 *
 *    
 */

vdeHInstance_t vdeInit(h263dOpen_t *param)
/* {{-output"vdeInit.txt"}} */
{
   vdeInstance_t *instance;

   vdeAssert(param);
   vdeAssert(!param->fRPS || (param->fRPS && param->numReferenceFrames > 1));

   instance = (vdeInstance_t *) 
      vdeMalloc(sizeof(vdeInstance_t) + param->freeSpace);
   if (instance == NULL)
      goto errInstanceAllocation;
   memset(instance, 0, sizeof(vdeInstance_t));

   if (param->lumWidth % 16)
      param->lumWidth = (param->lumWidth / 16 + 1) * 16;
   
   if (param->lumHeight % 16)
      param->lumHeight = (param->lumHeight / 16 + 1) * 16;

   if (vdeImsOpen(&instance->imageStore, param->numPreallocatedFrames, 
      param->lumWidth, param->lumHeight) < 0)
      goto errVdeImsOpen;

   instance->vdcHInstance = vdcOpen(&instance->imageStore, 
      param->fRPS ? param->numReferenceFrames : 1,
      (void *) instance);
   if (!instance->vdcHInstance)
      goto errVdcOpen;

   if (vdeFrtOpen(&instance->renStore) < 0)
      goto errRenStore;

   if (vdeFrtOpen(&instance->startCallbackStore) < 0)
      goto errStartCallbackStore;

   if (vdeFrtOpen(&instance->endCallbackStore) < 0)
      goto errEndCallbackStore;


   instance->lumWidth = param->lumWidth;
   instance->lumHeight = param->lumHeight;

   return (vdeHInstance_t) instance;

   /* Error cases (release resources in reverse order) */

errEndCallbackStore:
    
   vdeFrtClose(&instance->startCallbackStore);
errStartCallbackStore:

   vdeFrtClose(&instance->renStore);
errRenStore:

   vdcClose(instance->vdcHInstance);
errVdcOpen:

   vdeImsClose(&instance->imageStore);
errVdeImsOpen:

   vdeDealloc(instance);
errInstanceAllocation:

   return NULL;
}


/* {{-output"vdeIsINTRA.txt"}} */
/*
 * vdeIsINTRA
 *    
 *
 * Parameters:
 *    hInstance                  handle of instance data
 *    frameStart                 pointer to memory chunk containing a frame
 *    frameLength                number of bytes in frame
 *
 * Function:
 *    This function returns 1 if the passed frame is an INTRA frame.
 *    Otherwise the function returns 0.
 *
 * Note:
 *    This function does not use vdeque services since it is intended to be
 *    used in non-thread version of the codec only.
 *
 * Returns:
 *    See above.
 */

int vdeIsINTRA(
   vdeHInstance_t hInstance,
   void *frameStart,
   unsigned frameLength)
/* {{-output"vdeIsINTRA.txt"}} */
{
   vdeInstance_t *instance = (vdeInstance_t *) hInstance;

   vdeAssert(instance != NULL);

   /* Note: We assume that vdeDetermineStreamType has been called to
      decide whether the stream is MPEG-4 or H.263 */
      
   if (instance->fMPEG4)
      return vdcIsMPEGINTRA(instance->vdcHInstance, frameStart, frameLength);
   else
      return vdcIsINTRA(instance->vdcHInstance, frameStart, frameLength);
}


/* {{-output"vdeLoad.txt"}} */
/*
 * vdeLoad
 *    
 *
 * Parameters:
 *    rendererFileName           file from which to get renderer functions
 *
 * Function:
 *    This function initializes this module.
 *
 *    Renderer functions are dynamically loaded from the given file.
 *    If rendererFileName is NULL, renderer functions are expected to be
 *    statically linked.
 *
 * Returns:
 *    VDE_OK                     if the function was successful
 *    VDE_ERROR                  indicating a general error
 *
 *    
 */

int vdeLoad(const char * /*rendererFileName*/)
/* {{-output"vdeLoad.txt"}} */
{

   if (vdcLoad() < 0)
      return VDE_ERROR;

   return VDE_OK;
}




/* {{-output"vdeSetInputBuffer.txt"}} */
/*
 * vdeSetInputBuffer
 *    
 *
 * Parameters:
 *    hInstance                  handle of instance data
 *    buffer                     a new bit buffer to use
 *
 * Function:
 *    This function sets the bit buffer to use for decoding.
 *    It is intended that this function is used mainly in applications
 *    where frames are provided one by one from applications.
 *    Some upper level function must create a bit buffer for each frame,
 *    pass a pointer to the created bit buffer to the decoder using this
 *    function and then decode the frame.
 *
 * Returns:
 *    VDE_OK                     if the function was successful
 *    VDE_ERROR                  indicating a general error
 *
 *    
 */

int vdeSetInputBuffer(vdeHInstance_t hInstance, bibBuffer_t *buffer)
/* {{-output"vdeSetInputBuffer.txt"}} */
{
   vdeInstance_t *instance = (vdeInstance_t *) hInstance;

   if (instance == NULL)
      return VDE_ERROR;

   instance->inBuffer = buffer;

   return VDE_OK;
}



int vdeSetOutputBuffer(vdeHInstance_t hInstance, bibBuffer_t *buffer)
/* {{-output"vdeSetOutputBuffer.txt"}} */
{
   vdeInstance_t *instance = (vdeInstance_t *) hInstance;

   if (instance == NULL)
      return VDE_ERROR;

   instance->outBuffer = buffer;

   return VDE_OK;
}

int vdeSetBufferEdit(vdeHInstance_t hInstance, bibBufferEdit_t *bufEdit)
/* {{-output"vdeSetOutputBuffer.txt"}} */
{
   vdeInstance_t *instance = (vdeInstance_t *) hInstance;

   if (instance == NULL)
      return VDE_ERROR;

   instance->bufEdit = bufEdit;

   return VDE_OK;
}

int vdeSetVideoEditParams(vdeHInstance_t hInstance, int aColorEffect, TBool aGetDecodedFrame, 
                          TInt aColorToneU, TInt aColorToneV)
{
   vdeInstance_t *instance = (vdeInstance_t *) hInstance;

   if (instance == NULL)
      return VDE_ERROR;

   instance->iColorEffect = aColorEffect;
   instance->iGetDecodedFrame = aGetDecodedFrame;
   instance->iColorToneU = aColorToneU;
   instance->iColorToneV = aColorToneV;

   return VDE_OK;
}









/* {{-output"vdeShutDown.txt"}} */
/*
 * vdeShutDown
 *    
 *
 * Parameters:
 *    hInstance                  handle of instance data
 *
 * Function:
 *    This function has to be called in the end of each video sequence.
 *    It frees the resources (the VDE instance) allocated by vdeInit.
 *
 * Returns:
 *    VDE_OK                     if the closing was successful
 *    VDE_ERROR                  indicating a general error
 *
 *    
 */

int vdeShutDown(vdeHInstance_t hInstance)
/* {{-output"vdeShutDown.txt"}} */
{
   int retValue = VDE_OK;
   vdeInstance_t *instance = (vdeInstance_t *) hInstance;

   if (vdcClose(instance->vdcHInstance) < 0)
      retValue = VDE_ERROR;

   if (vdeFrtClose(&instance->endCallbackStore) < 0)
      retValue = VDE_ERROR;

   if (vdeFrtClose(&instance->startCallbackStore) < 0)
      retValue = VDE_ERROR;

   if (vdeFrtClose(&instance->renStore) < 0)
      retValue = VDE_ERROR;

   if (vdeImsClose(&instance->imageStore) < 0)
      retValue = VDE_ERROR;

   vdeDealloc(instance);

   return retValue;
}

// End of File
