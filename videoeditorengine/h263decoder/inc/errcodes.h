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
* Error code definitions.
*
*/


#ifndef _ERRCODES_H_
#define _ERRCODES_H_

/*
 * Defines
 */

/*
 * Common errors for VDC and FCC
 */

#define ERR_COM_NULL_POINTER 100


/* 
 * Bit buffer module errors 
 */

#define ERR_BIB_STRUCT_ALLOC 1000
#define ERR_BIB_BUFFER_ALLOC 1001
#define ERR_BIB_FILE_READ 1002
#define ERR_BIB_NOT_ENOUGH_DATA 1003
#define ERR_BIB_ALREADY_OPENED 1004
#define ERR_BIB_FILE_OPEN 1005
#define ERR_BIB_ALREADY_CLOSED 1006
#define ERR_BIB_FILE_CLOSE 1007
#define ERR_BIB_NUM_BITS 1008
#define ERR_BIB_FILE_NOT_OPEN 1009
#define ERR_BIB_ILLEGAL_SIZE 1010
#define ERR_BIB_CANNOT_REWIND 1011
#define ERR_BIB_FEC_RELOCK 1050


/* 
 * Variable length code module errors 
 */

#define ERR_VLC_MCBPC_INTRA 8500
#define ERR_VLC_CBPY 8501
#define ERR_VLC_MCBPC_INTER 8502
#define ERR_VLC_MVD 8503


/* 
 * Motion vector count module errors 
 */

#define ERR_MVC_ALLOC1 1200
#define ERR_MVC_ALLOC2 1201
#define ERR_MVC_NO_PREV_MB 1202
#define ERR_MVC_NEIGHBOUR_NOT_VALID 1203
#define ERR_MVC_PREV_NOT_VALID 1204
#define ERR_MVC_PREV_NOT_CODED 1205
#define ERR_MVC_PREV_INTRA 1206
#define ERR_MVC_CURR_NOT_VALID 1207
#define ERR_MVC_CURR_NOT_CODED 1208
#define ERR_MVC_CURR_INTRA 1209
#define ERR_MVC_MVDX_ILLEGAL 1250
#define ERR_MVC_MODE_ILLEGAL 1251
#define ERR_MVC_X_ILLEGAL 1252
#define ERR_MVC_Y_ILLEGAL 1253
#define ERR_MVC_MAX_X_ILLEGAL 1254
#define ERR_MVC_TIME_ILLEGAL 1255
#define ERR_MVC_MVX_ILLEGAL 1256
#define ERR_MVC_MVY_ILLEGAL 1257
#define ERR_MVC_MVDY_ILLEGAL 1258
#define ERR_MVC_MVPTR 1260



/* 
 * Block module errors 
 */

#define ERR_BLC_TCOEF 8504
#define ERR_BLC_LEVEL 5001
#define ERR_BLC_TOO_MANY_COEFS 5002
#define ERR_BLC_MV_OUTSIDE_PICT 5003


/* 
 * Core module errors 
 */

/* General errors */
#define ERR_VDC_MEMORY_ALLOC 2080
#define ERR_VDC_NO_INTRA 2081
#define ERR_VDC_INITIALIZE 2082
#define ERR_VDC_CORRUPTED_INTRA 2083
#define ERR_VDC_UNEXPECTED_FATAL_ERROR 2084

/* Picture Layer errors */
#define ERR_VDC_PTYPE_BIT1 2048
#define ERR_VDC_PTYPE_BIT2 2049
#define ERR_VDC_PTYPE_BIT9_AND_BIT13_MISMATCH 2050
#define ERR_VDC_SOURCE_FORMAT 2051
#define ERR_VDC_NO_SIZE 2052
#define ERR_VDC_ILLEGAL_PQUANT 2053
#define ERR_VDC_ILLEGAL_TRB 2054
#define ERR_VDC_ILLEGAL_PSPARE 2055
#define ERR_VDC_NO_PSC 2056
#define ERR_VDC_FORMAT_CHANGE 2057

/* Group of Blocks Layer errors */
#define ERR_VDC_ILLEGAL_GN 2112
#define ERR_VDC_ILLEGAL_GFID 2113
#define ERR_VDC_ILLEGAL_GQUANT 2114


/* 
 * Format Conversion Core errors 
 */

#define ERR_CON_UNKNOWN_MODE 3000
#define ERR_CON_CLIP_ALLOC 3001
#define ERR_CON_Y_TOO_LARGE 3002
#define ERR_CON_DITH_INDEX_ILLEGAL 3003
#define ERR_CON_GREY256_INDEX_ILLEGAL 3004
#define ERR_CON_UNKNOWN_BIT_DEPTH 3005
#define ERR_CON_NULL_PALETTE 3006
#define ERR_CON_PALETTE_EXISTS 3007
#define ERR_CON_ALLOC 3008
#define ERR_CON_NO_CUSTOM_PALETTE 3009
#define ERR_CON_WIN32_ERROR 3010

#define ERR_CON_OCT_ALLOC 3100
#define ERR_CON_OCT_NULL_PTR 3101
#define ERR_CON_OCT_INDEX_EXISTS 3102

#endif


// End of File
