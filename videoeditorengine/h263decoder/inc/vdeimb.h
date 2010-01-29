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
* A header file for the Video Decoder Engine Image Buffer abstract 
* data type.
*
*/


#ifndef _VDEIMB_H_
#define _VDEIMB_H_



/*
 * Includes
 */

#include "renapi.h"
#include "rendri.h"


/*
 * Defines
 */

/* Displaying status */
#define VDEIMB_NO_DISPLAYING 1
#define VDEIMB_WAITING 2
#define VDEIMB_DISPLAYED 4

/* Full-picture freeze status,
   see vdeImb_t for description of the definitions */
#define VDEIMB_FPF_REQUEST 2
#define VDEIMB_FPF_RELEASE 1
#define VDEIMB_FPF_NOT_SPECIFIED 0


/*
 * Structures and typedefs
 */

/* {{-output"vdeImb_t_info.txt" -ignore"*" -noCR}}
   vdeImb_t defines the structure of a single image buffer which is capable
   of storing one YUV 4:2:0 image and a number of paramaters as 
   side-information. The YUV image as well as the YUV to RGB conversion and
   displaying related parameters are kept in a renDrawItem_t structure.
   Decoder related parameters are stored in the vdeImb_t itself.
   One may directly refer to the member variables of the vdeImb_t structure.
   However, one should use the functions and macros in the Renderer Draw Item
   Interface for accessing the contents of the drawItem member of vdeImb_t.
   {{-output"vdeImb_t_info.txt"}} */

/* {{-output"vdeImb_t.txt"}} */
typedef struct {
   renDrawItem_t *drawItem;   /* renderer draw item which actually contains
                                 the frame memory (and some parameters) */

   int fReferenced;           /* non-zero if the image buffer is going to be
                                 used as a reference frame. Otherwise, zero. */

   int tr;                    /* the TR field of H.263 which is associated with
                                 the image buffer */

   
   int trRef;                 /* Referenced for a TR prediction,
                                 0 if the frame is a normal picture referenced
                                   once at maximum (in the next picture),
                                 1 if the frame is referenced multiple times
                                   in the future frames.
                                 This information is used to update reference
                                 picture data structures. It is neede at least
                                 in streaming applications. */

   int *yQuantParams;      /* luminance quantizer parameter for each 
                              macroblock in scan-order */
                              
   int *uvQuantParams;     /* chrominance quantizer parameter for each 
                              macroblock in scan-order */

} vdeImb_t;
/* {{-output"vdeImb_t.txt"}} */


/*
 * Function prototypes
 */

vdeImb_t *vdeImbAlloc(int width, int height, int fYuvNeeded);

int vdeImbCopyParameters(vdeImb_t *dstImb, const vdeImb_t *srcImb);

void vdeImbDealloc(vdeImb_t *imb);


/*
 * Macros
 */

/* {{-output"vdeImbYUV.txt"}} */
/*
 * vdeImbYUV
 *
 * Parameters:
 *    vdeImb_t *pimb             a pointer to image buffer
 *    u_char **ppy, **ppu, **ppv used to return the pointers to the top-left
 *                               corner of Y, U and V image buffers
 *                               respectively
 *    int *pWidth, *pHeight      used to return the width and height of
 *                               the luminance image in pixels
 *
 * Function:
 *    This function returns the Y, U and V pointers to the passed image buffer
 *    as well as the dimensions of the luminance frame of the image buffer.
 *
 * Returns:
 *    REN_OK                     if the function was successful
 *    REN_ERROR                  indicating a general error
 *
 */

#define vdeImbYUV(pimb, ppy, ppu, ppv, pWidth, pHeight) \
   renDriYUV((pimb)->drawItem, (ppy), (ppu), (ppv), (pWidth), (pHeight))
/* {{-output"vdeImbYUV.txt"}} */

#endif
// End of File
