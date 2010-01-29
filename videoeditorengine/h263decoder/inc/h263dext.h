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
* the Video Decoder Engine.
*
*/


#ifndef _H263DEXT_H_
#define _H263DEXT_H_


/*
 * Includes
 */

//#include "renapi.h"
#include "epoclib.h"

/*
 * Defines
 */

/* _WIN32_EXPLICIT has to be defined before anything else. */

/* Return codes */
#define H263D_OK_BUT_NOT_CODED   4     /* not coded frame, 
                                          copied to output as is, 
                                          no decoded output generated */  
#define H263D_OK_BUT_BIT_ERROR   3     /* Bit errors detected but frame decoded */
#define H263D_OK_BUT_FRAME_USELESS \
                                 2     /* function behaved normally, but no output
                                          was produced due to too corrupted frame */
#define H263D_OK_EOS             1     /* End-of-Stream has been reached */
#define H263D_OK                 0     /* the function was successful */
#define H263D_ERROR             -1     /* indicates a general error */
#define H263D_ERROR_NO_INTRA    -2     /* No INTRA frame has been decoded, 
                                          decoding cannot be started */
#define H263D_ERROR_HALTED      -3     /* no further decoding possible */
/* Note: In practice, there is no difference between H263D_ERROR and 
   H263D_ERROR_HALTED. The caller should react to both error messages
   in the same way. They both exist due to compatibility with older
   versions of the decoder. */


/* Frame type scope. See h263dFrameType_t (in this file) for descriptions. */
#define H263D_FTYPE_ALL       0xffffffff
#define H263D_FTYPE_NDEF      0xffffffff
#define H263D_FTYPE_DEF       0
#define H263D_FTYPE_SIZE      0x00000001
#define H263D_FTYPE_SNAPSHOT  0x00000002


/* Mode values used in h263dSetStartOrEndCallback to select if the function
   scope is in the start of the frame (H263D_CB_START) 
   or in the end the frame (H263D_CB_END),
   or both (H263D_CB_START | H263D_CB_END). */
#define H263D_CB_START     0x00000001
#define H263D_CB_END       0x00000002


/* H.263 Annex N submodes */
#define H263D_BC_MUX_MODE_SEPARATE_CHANNEL   1  /* Separate Logical Channel 
                                                   submode*/
#define H263D_BC_MUX_MODE_VIDEO              2  /* VideoMux submode */


/* Error resilience features, see h263dSetErrorResilience for details */
#define H263D_ERD_FEATURE_STATUS 1
#define H263D_ERD_FEATURE_CHECK_ALL 2
#define H263D_ERD_FEATURE_DISCARD_CORRUPTED 3

#define H263D_ERD_INTRA_DCT_DOMAIN 0
#define H263D_ERD_INTRA_PIXEL_DOMAIN 1

#define H263D_ERD_INTER_NO_MOTION_COMPENSATION 0
#define H263D_ERD_INTER_MOTION_COMPENSATION 1


/* Output types, see h263dSetOutputFile for more information */
#define H263D_OUT_FILE_MODE_FRAME_BY_FRAME      1
#define H263D_OUT_FILE_MODE_ONE_PER_COMPONENT   2
#define H263D_OUT_FILE_MODE_ONE_FOR_ALL         3



/* Temporal/computational scalability levels, 
   see h263dSetTemporalScalabilityLevel for more details */
#define H263D_LEVEL_ALL_FRAMES   0
#define H263D_LEVEL_INTRA_FRAMES 1


/* Calling convention for exported functions */
#ifndef H263D_CC
   #define H263D_CC   
#endif


/* Calling convention for callback functions declared in the decoder */
#ifndef H263D_CALLBACK   
   #define H263D_CALLBACK
#endif


/* Declaration specifier for exported functions */
#ifndef H263D_DS
   #define H263D_DS
#endif


/*
 * Structs and typedefs
 */

/* size_t */
   typedef unsigned int size_t;


/* {{-output"h263dHInstance_t_info.txt" -ignore"*" -noCR}}
   h263dHInstance_t is used as a unique identifier of a H.263 Video Decoder
   instance.
   The type can be casted to u_int32 or void pointer and then back to
   h263dHInstance_t without any problems.
   {{-output"h263dHInstance_t_info.txt"}} */

/* {{-output"h263dHInstance_t.txt"}} */
typedef void * h263dHInstance_t;
/* {{-output"h263dHInstance_t.txt"}} */


/* {{-output"h263dFrameType_t_info.txt" -ignore"*" -noCR}}
   This structure is used to define a frame type scope for some setting
   functions.
   Currently, there are three scopes: the default scope (H263D_FTYPE_DEF),
   the frame size scope (H263D_FTYPE_SIZE) and the snapshot scope 
   (H263D_FTYPE_SNAPSHOT). If a setting is defined for a particular size, 
   all frames having that size will be handled according to the setting, 
   i.e. the frame size scope overrides the default scope. Later on, it may 
   be possible to add more frame types scopes, like the enchancement layer 
   number or the snapshot tag (Annex L.8. of the H.263 recommendation). 
   Then, the scope order must also be defined. For example, if there are 
   two settings, one for a particular size and one for a particular enhancement 
   layer. If a frame fulfills both scopes, one has to know which scope has 
   the priority, e.g. that the frame has to be handled according to the 
   enchancement layer setting. There can also be a combination of scopes, 
   e.g. some setting may be defined for a particular frame size and 
   enhancement layer.
   {{-output"h263dFrameType_t_info.txt"}} */

/* {{-output"h263dFrameType_t.txt"}} */
typedef struct {
   u_int32 scope;       /* the scope of the frame type setting:
                           H263D_FTYPE_ALL
                              This setting overrides all previous settings,
                              i.e. it is valid for all frame types.
                           H263D_FTYPE_DEF
                              is used to define a setting for frame types
                              which do not have a specific setting of their
                              own.
                           H263D_FTYPE_SIZE
                              is used to define a setting for a particular
                              frame size.

                           H263D_FTYPE_SNAPSHOT
                              is used to define a setting for snapshot
                              frames.

                           For internal use only:
                           H263D_FTYPE_NDEF
                              is used in querying functions to solve the scope
                              depending on the parameters which are used.
                              All parameters must be set.
                        */

   int width;
   int height;          /* width and height of the frame which is in the scope
                           of the setting. These parameters are valid
                           only if scope indicates H263D_FTYPE_SIZE. */
   u_char fSnapshot;    /* snapshot flag, valid only if scope is 
                           H263D_FTYPE_SNAPSHOT */
} h263dFrameType_t;
/* {{-output"h263dFrameType_t.txt"}} */


/* Used in h263dOpen_t to pass a callback function to call when decoding
   has to stopped. */
typedef void (H263D_CALLBACK *h263dDecodingFinishedCallback_t) (void *);



/* Prototype for callback function related to h263dSetStartOrEndCallback.
   See the function description for more details. */
typedef void (H263D_CALLBACK *h263dStartOrEndCallback_t) 
   (u_int32, u_int32, void *);

/* Prototype for callback function related to h263dSetStartOrEndSnapshotCallback.
   See the function description for more details. */
typedef void (H263D_CALLBACK *h263dStartOrEndSnapshotCallback_t) (u_int32, u_int32, void *);

/* Prototype for callback function related to h263dSetReportPictureSizeCallback.
   See the function description for more details. */
typedef void (H263D_CALLBACK *h263dReportPictureSizeCallback_t) (void *, int, int);
                              
typedef struct {
   int fExist;          /* 1 or 0 */
   u_int8 data[255];    /* header data */
   int length;          /* length of header data */
} h263dMPEG4Header_t;

/* {{-output"h263dOpen_t_info.txt" -ignore"*" -noCR}}
   This structure is used with the h263dOpen function to give the necessary
   data for opening a new H.263 Video Decoder Engine instance.
   {{-output"h263dOpen_t_info.txt"}} */

/* {{-output"h263dOpen_t.txt"}} */
typedef struct {
   int numPreallocatedFrames;    /* Number of preallocated frame memories */

   int lumWidth;                 /* Size of preallocated frame memories */
   int lumHeight;

   int fRPS;                     /* 1 = Reference Picture Selection mode in use */

   int numReferenceFrames;       /* number of reference frames in RPS mode */

   h263dDecodingFinishedCallback_t decodingFinishedCallback;
                                 /* callback function to call when no data has
                                    been decoded for a while */

   int decodingFinishedIdleTimeInMSec;
                                 /* number of milliseconds to go by until
                                    decodingFinishedCallback is called */

   h263dMPEG4Header_t mpeg4Header;
                                 /* MPEG-4 header received via H.245 */

   size_t freeSpace;             /* Used internally */
} h263dOpen_t;
/* {{-output"h263dOpen_t.txt"}} */


/*
 * Function prototypes
 */

H263D_DS int H263D_CC h263dFree(
   void);

H263D_DS int H263D_CC h263dLoad(
   void);

H263D_DS int H263D_CC h263dSetStartOrEndCallback(
   h263dHInstance_t hInstance,
   u_int32 mode, 
   h263dFrameType_t *frameType,
   h263dStartOrEndCallback_t callback, 
   u_int32 param);


H263D_DS int H263D_CC h263dSetReportPictureSizeCallback(
   h263dHInstance_t hInstance, 
   h263dReportPictureSizeCallback_t callback);

#endif
// End of File
