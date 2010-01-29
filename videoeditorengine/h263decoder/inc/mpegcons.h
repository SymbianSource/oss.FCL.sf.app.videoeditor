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
* Definition of constants used in MPEG-4 bitstream decoding.
*
*/


#ifndef _MPEGCONS_H_
#define _MPEGCONS_H_

#include "epoclib.h"

/*
 * Defines
 */

/* end of bitstream code 
   inserted by the decoder if no new input buffer is got (bibNewBuffer()) */
 
#define MP4_EOB_CODE                0x1B1
#define MP4_EOB_CODE_LENGTH         32

/* session layer and vop layer start codes */
 
#define MP4_VOS_START_CODE          0x1B0
#define MP4_VOS_START_CODE_LENGTH   32

#define MP4_VO_START_CODE           0x1B5
#define MP4_VO_START_CODE_LENGTH    32

#define MP4_VID_START_CODE          0x8
#define MP4_VID_START_CODE_LENGTH   27
#define MP4_VID_ID_CODE_LENGTH      5
 
#define MP4_VOL_START_CODE          0x12  
#define MP4_VOL_START_CODE_LENGTH   28
#define MP4_VOL_ID_CODE_LENGTH      4
 
#define MP4_GROUP_START_CODE        0x1B3
#define MP4_GROUP_START_CODE_LENGTH 32     
 
#define MP4_VOP_START_CODE          0x1B6 
#define MP4_VOP_START_CODE_LENGTH   32

#define MP4_USER_DATA_START_CODE    0x1B2
 
/* motion and resync markers used in error resilient mode  */
 
#define MP4_DC_MARKER                 438273
#define MP4_DC_MARKER_LENGTH          19
 
#define MP4_MOTION_MARKER_COMB        126977
#define MP4_MOTION_MARKER_COMB_LENGTH 17
 
#define MP4_RESYNC_MARKER             1
#define MP4_RESYNC_MARKER_LENGTH      17
 
#endif
// End of File
