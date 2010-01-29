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
* Header file for Bit Buffer module. 
*
*/


#ifndef _BIBLIN_H_
#define _BIBLIN_H_

#include "epoclib.h"

/*
 * Defines
 */

/* Error codes */
/* Obsolete error codes are not used anymore and are defined only to maintain
   backwards compatibility with older versions of the file. */
#define ERR_BIB_STRUCT_ALLOC 1000      /* If a structure allocation failed */
#define ERR_BIB_BUFFER_ALLOC 1001      /* Obsolete */
#define ERR_BIB_FILE_READ 1002         /* Obsolete */
#define ERR_BIB_NOT_ENOUGH_DATA 1003   /* If the number of bits requested from
                                          the buffer is greater than the number
                                          of bits available in the buffer */
#define ERR_BIB_ALREADY_OPENED 1004    /* Obsolete */
#define ERR_BIB_FILE_OPEN 1005         /* Obsolete */
#define ERR_BIB_ALREADY_CLOSED 1006    /* Obsolete */
#define ERR_BIB_FILE_CLOSE 1007        /* Obsolete */
#define ERR_BIB_NUM_BITS 1008          /* Obsolete */
#define ERR_BIB_FILE_NOT_OPEN 1009     /* Obsolete */
#define ERR_BIB_ILLEGAL_SIZE 1010      /* Obsolete */
#define ERR_BIB_CANNOT_REWIND 1011     /* If the number of bits requested to be
                                          rewinded is greater than the number
                                          of bits available in the buffer */
#define ERR_BIB_BUFLIST 1012           /* If the internal buffer list has
                                          been corrupted */
#define ERR_BIB_TOO_SMALL_BUFFER 1013  /* Obsolete */
#define ERR_BIB_FEC_RELOCK 1050        /* Obsolete */
#define ERR_BIB_PSC_FOUND 1060         /* Obsolete */


/*
 * Structs and typedefs
 */



/* {{-output"bibBuffer_t_info.txt" -ignore"*" -noCR}}
 * The bibBuffer_t data type is a structure containing all the necessary data
 * for a bit buffer instance (except for the actual data buffer). This
 * structure is passed to all of the bit buffer functions.
 * {{-output"bibBuffer_t_info.txt"}}
 */

/* {{-output"bibBuffer_t.txt"}} */



enum CopyMode {
     CopyNone        = 0,
     CopyWhole       = 1,
     CopyWithEdit    = 2,
     EditOnly        = 3
};

typedef struct bibEditParams_s {

   int StartByteIndex;      // start byte position where data is to be written 
   int StartBitIndex;       // start bit position where data is to be written 
   int curNumBits;      // number of bits that need to be replaced 
   int newNumBits;      // number of bits to be written 
   int newValue;      // the value to be written 

} bibEditParams_t; 

typedef struct bibBufferEdit_s {

    CopyMode copyMode; 
    int numChanges; 
    bibEditParams_t *editParams;

} bibBufferEdit_t; 




typedef struct bibBuffer_s {
   u_char *baseAddr;       /* the start address of the buffer */

   unsigned size;          /* the size of the buffer in bytes */

   unsigned getIndex;      /* an index to the buffer where data was last got */

   int bitIndex;           /* an index to the byte pointed by getIndex + 1 */

   u_int32 bitsLeft;       /* the number of bits currently in the buffer */

   u_int32 numBytesRead;   /* the total number of bytes read */

   int error;               /* stores possible error code */

} bibBuffer_t;
/* {{-output"bibBuffer_t.txt"}} */

#ifdef DEBUG_OUTPUT
extern bibBuffer_t * buffer_global;
#endif

/* typedefs for bibFlushBits, bibGetBits, and bibShowBits function types */
typedef void (*bibFlushBits_t) (int, bibBuffer_t *, int *, int *, int16 *);
typedef u_int32 (*bibGetBits_t) (int, bibBuffer_t *, int *, int *, int16 *);
typedef u_int32 (*bibShowBits_t) (int, bibBuffer_t *, int *, int *, int16 *);

/*
 * External macros
 */

/*
    * bibNumberOfBitsLeft
    *
    * Parameters:
    *    bibBuffer_t *buffer        input bit buffer instance
    *
    * Function:
    *    This macro returns the number of bits which are left to be read
    *    from the current position.
    *
    * Returns:
    *    See above.
    */

   #define bibNumberOfBitsLeft(buffer) \
      ((buffer)->bitsLeft)

/*
 * External function prototypes
 */

bibBuffer_t *bibCreate(void *srcBuffer, unsigned srcBufferLength,
   int16 *errorCode);

void bibDelete(bibBuffer_t *buffer, int16 *errorCode);

u_int32 bibNumberOfFlushedBits(bibBuffer_t *buffer);

u_int32 bibNumberOfFlushedBytes(bibBuffer_t *buffer);

u_int32 bibNumberOfRewBits(bibBuffer_t *buffer);

void bibRewindBits(u_int32 numberOfBits, bibBuffer_t *buffer, int16 *errorCode);


/* 
 * Prototypes for bibFlushBits/bibGetBits/bibShowBits 
 */

void bibFlushBits(int numberOfBits, bibBuffer_t *buffer);
u_int32 bibGetBits(int numberOfBits, bibBuffer_t *buffer);
u_int32 bibShowBits(int numberOfBits, bibBuffer_t *buffer);

inline void bibFlushBits(int numberOfBits, bibBuffer_t *buffer, int *numberOfBitsGot, int * /*bitErrorIndication*/, int16 * /*errorCode*/)
{
    *numberOfBitsGot = numberOfBits;
    bibFlushBits(numberOfBits, buffer);
}

inline u_int32 bibGetBits(int numberOfBits, bibBuffer_t *buffer, int *numberOfBitsGot, int * /*bitErrorIndication*/, int16 * /*errorCode*/)
{
    *numberOfBitsGot = numberOfBits;
    return bibGetBits(numberOfBits, buffer);
}

inline u_int32 bibShowBits(int numberOfBits, bibBuffer_t *buffer, int *numberOfBitsGot, int * /*bitErrorIndication*/, int16 * /*errorCode*/)
{
    *numberOfBitsGot = numberOfBits;
    return bibShowBits(numberOfBits, buffer);
}

#define bibFlushBitsFromBuffer bibFlushBits
#define bibGetBitsFromBuffer bibGetBits
#define bibShowBitsFromBuffer bibShowBits


bibBufferEdit_t *bibBufferEditCreate(int16 *errorCode);
void bibBufEditDelete(bibBufferEdit_t *bufEdit, int16 *errorCode);

// copy from input buffer to output buffer in various copy modes (with or without editing)
void CopyStream(bibBuffer_t *SrcBuffer,bibBuffer_t *DestBuffer,bibBufferEdit_t *bufEdit, 
								int ByteStart,int BitStart);
// copy from input buffer to output buffer (without editing)
void CopyBuffer(bibBuffer_t *SrcBuffer,bibBuffer_t *DestBuffer, 
								int ByteStart,int BitStart, int ByteEnd, int BitEnd);
// copy from BufferEdit to output buffer (no copying; rather, inserting code into output buffer)
void CopyBufferEdit(bibBuffer_t *SrcBuffer, bibBuffer_t *DestBuffer, 
										bibEditParams_t *edParam, int updateSrcBufferStats=1);
// insert correct IntraDC values for H.263 chrominance blocks in output buffer
void ResetH263IntraDcUV(bibBuffer_t *DestBuffer, int uValue, int vValue); 
// insert correct IntraDC values for MPEG-4 chrominance blocks in output buffer
void ResetMPEG4IntraDcUV(bibBuffer_t *DestBuffer, int IntraDC_size); 

	/* 
			SrcValue		the source value from which bits are to be extacted 
			MaxNumBits	the length in bits of the source value
			StartBit		the index of the starting bit form where data is to be retrieved
			getBits			the number of bits to be retrieved 
	*/
u_int32 bibGetBitsFromWord(u_int32 SrcValue, u_int32 getBits, u_int32 *StartBit, 
												u_int32 MaxNumBits);
void bibForwardBits(u_int32 numberOfBits, bibBuffer_t *buffer);
void bibStuffBits(bibBuffer_t *buffer);
void bibStuffBitsMPEG4(bibBuffer_t *inBuffer, bibBuffer_t *outBuffer, bibBufferEdit_t *bufEdit, 
											 int *StartByteIndex, int *StartBitIndex, int updateSrcBufferStats);


#endif
// End of File
