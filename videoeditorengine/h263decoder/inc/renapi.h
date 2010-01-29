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
* A header file for external usage of the services provided by
* the Video Renderer engine.
*
*/


#ifndef _RENAPI_H_
#define _RENAPI_H_

/*
 * Includes
 */




/*
 * Defines
 */

/* Return values */
#define REN_ERROR_BAD_IMAGE_SIZE -4
#define REN_ERROR_BAD_FORMAT -3
#define REN_ERROR_HALTED -2
#define REN_ERROR -1
#define REN_OK 1
#define REN_OK_PLAYING_COMPLETE 2
#define REN_OK_RESOURCE_LEAK 3

/* Defines for the flags member of renExtDrawParam_t */
#define REN_EXTDRAW_NEW_SOURCE 1
#define REN_EXTDRAW_COPY_NOT_CODED 2
#define REN_EXTDRAW_NO_STATUS_CALLBACK 32



/*
 * Structs and typedefs
 */




/* {{-output"renbmihi.txt" -ignore"*" -noCR}}
   renBitmapInfoHeader_t is used to store the header information of
   (device-independent) bitmaps such as individual video frames.
   {{-output"renbmihi.txt"}} */

/* {{-output"renbmiht.txt"}} */

typedef struct {
   u_int32 biSize;
   int32 biWidth;
   int32 biHeight;
   u_int32 biSizeImage;
} renBitmapInfoHeader_t;
/* {{-output"renbmiht.txt"}} */




/* {{-output"renExtDrawParam_t_info.txt" -ignore"*" -noCR}}
   renExtDrawParam_t is used in a renDrawItem_t structure to provide
   additional information which cannot be included in a renDrawParam_t
   structure.
   {{-output"renExtDrawParam_t_info.txt"}} */

/* {{-output"renExtDrawParam_t.txt"}} */
typedef struct {
   u_int32 size;        /* The size of this structure in bytes,
                           if 0, no macroblock information is used
                           and the whole frame is converted */

   u_int32 flags;       /* REN_EXTDRAW_NEW_SOURCE = use this frame as
                           a new source for macroblock copying
                           (for subsequent frames until a new source
                           frame appears),
                           REN_EXTDRAW_COPY_NOT_CODED = use copying of
                           not coded macroblocks from the source frame,
                           REN_EXTDRAW_SNAPSHOT = mark this frame as
                           a snapshot
                           REN_EXTDRAW_NO_STATUS_CALLBACK = no status callback
                           function is called after this frame has been
                           processed
                           */

   u_int32 rate;        /* Frames / second = rate / scale, */
   u_int32 scale;       /* These values replace the current values and affect
                           the subsequent frames. */

   /* The values in this section are valid only if
      the REN_EXTDRAW_COPY_NOT_CODED flag is on. */
   int numOfMBs;        /* The number of macroblocks (16 x 16 blocks) in the
                           frame. */
   int numOfCodedMBs;   /* The number of coded macroblocks in the frame
                           (compared to the previous reference frame) */
   u_char *fCodedMBs;   /* one-dimensional array of u_char
                           flags indicating if the corresponding macroblock
                           in the normal scan-order is coded (1) or not (0) */

} renExtDrawParam_t;
/* {{-output"renExtDrawParam_t.txt"}} */


#define RENDRAWPARAM_NOSTATUSCALLBACK 0x00000001L
                           /* Do not call the status callback function after
                              the processing of this frame */

/* {{-output"renDrawParam_t_info.txt" -ignore"*" -noCR}}
   renDrawParam_t contains parameters for drawing video data to the screen.
   This structure is used with the renDraw function.
   renDrawParam_t is compatible with the ICDRAW type introduced by
   the Win32 Video Compression Manager. The lpData member of renBasicDrawParam_t
   must point to an address where the upper-left corner pixel of the Y frame
   to be shown lies. The Y frame data must be in scanning order. The U frame
   data must follow the Y frame data in scanning order, and similarly
   the V frame data must follow the U frame data.
   {{-output"renDrawParam_t_info.txt"}} */

/* {{-output"renDrawParam_t.txt"}} */
typedef struct {
   u_int32 dwFlags;     /* Flags */
   void *lpFormat;      /* Pointer to a renBitmapInfoHeader_t structure */
   void *lpData;        /* Address of the data to render */
   u_int32 cbData;      /* Ignored */
   int32 lTime;         /* The frame number telling the time when this frame
                           should be drawn */
} renDrawParam_t;
/* {{-output"renDrawParam_t.txt"}} */



/* {{-output"renDrawItem_t_info.txt" -ignore"*" -noCR}}
   renDrawItem_t contains parameters for drawing video image to the screen.
   In addition, it holds the returning information for the image.
   This structure is used with the renDraw function.
   {{-output"renDrawItem_t_info.txt"}} */

/* {{-output"renDrawItem_t.txt"}} */
typedef struct {
   renDrawParam_t param;
                        /* Basic, ICDRAW compatible parameters */

   renExtDrawParam_t extParam;
                        /* Extended parameters */

} renDrawItem_t;
/* {{-output"renDrawItem_t.txt"}} */




#endif
// End of File
