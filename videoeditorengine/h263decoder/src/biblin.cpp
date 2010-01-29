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
* Bit buffer reading operations.
*
*/




/*
* Includes
*/

#include "h263dConfig.h"
#include "Biblin.h"
#include "debug.h"

#ifdef DEBUG_OUTPUT
bibBuffer_t * buffer_global;
#endif


/*
* Definitions
*/

#define bibMalloc malloc
#define bibCalloc calloc
#define bibRealloc realloc
#define bibDealloc free

#ifndef bibAssert
#define bibAssert(exp) assert(exp);
#endif


/*
* Local function prototypes
*/

static void bibInitialize(
                                                    bibBuffer_t *bibBuffer,
                                                    void *srcBuffer,
                                                    unsigned srcBufferLength,
                                                    int16 *errorCode);




/*
* Imported variables
*/



/*
* Global functions
*/

/* {{-output"bibCreate.txt"}} */
/*
*
* bibCreate
*    
*
* Parameters:
*    srcBuffer                  buffer containing the data
*    srcBufferLength            the length of the buffer
*    errorCode                  error code
*
* Function:
*    This function creates a bit buffer from a buffer given as a parameter.
*
* Returns:
*    The bibCreate function returns a pointer to a bibBuffer_t
*    structure. If the function is unsuccessful NULL is returned.
*
* Error codes:
*    ERR_BIB_STRUCT_ALLOC       if the bit buffer structure could not be
*                               allocated
*
*/

bibBuffer_t *bibCreate(void *srcBuffer, unsigned srcBufferLength, int16 *errorCode)
/* {{-output"bibCreate.txt"}} */
{
    bibBuffer_t *bibBuffer = NULL;
    int16 tmpError = 0;
    
    bibBuffer = (bibBuffer_t *) bibMalloc(sizeof(bibBuffer_t));
    if (bibBuffer == NULL) {
        *errorCode = ERR_BIB_STRUCT_ALLOC;
        deb("bibCreate: ERROR - bibMalloc failed.\n");
        return NULL;
    }
    
    bibInitialize(bibBuffer, srcBuffer, srcBufferLength, &tmpError);
    if (tmpError) {
        bibDealloc(bibBuffer);
        return NULL;
    }
    
#ifdef DEBUG_OUTPUT
    debLoad("deb_dec.log");
    buffer_global = bibBuffer;
#endif
    
    return bibBuffer;
}




bibBufferEdit_t *bibBufferEditCreate(int16 *errorCode)
/* {{-output"bibCreate.txt"}} */
{
    bibBufferEdit_t *bufEdit = NULL;    
    
    bufEdit = (bibBufferEdit_t *) bibMalloc(sizeof(bibBufferEdit_t));
    if (bufEdit == NULL) {
        *errorCode = ERR_BIB_STRUCT_ALLOC;
        deb("bibBufferEditCreate: ERROR - bibMalloc failed.\n");
        return NULL;
    }
     bufEdit->copyMode = CopyWhole/*CopyWhole*/;
     bufEdit->numChanges=0;
     bufEdit->editParams=NULL;
     
   return bufEdit;
}




/* {{-output"bibDelete.txt"}} */
/*
*
* bibDelete
*    
*
* Parameters:
*    buffer                     a pointer to a bit buffer structure
*    errorCode                  error code
*
* Function:
*    The bibDelete function frees the memory allocated for the buffer.
*
* Returns:
*    Nothing
*
* Error codes:
*    None
*
*/

void bibDelete(bibBuffer_t *buffer, int16 * /*errorCode*/)
/* {{-output"bibDelete.txt"}} */
{
    if (buffer) {
        // note that the BaseAddr is just a reference to memory allocated elsewhere
        // and there is no other dynamic allocations inside buffer
        bibDealloc((void *) buffer);
    }
}



void bibBufEditDelete(bibBufferEdit_t *bufEdit, int16 * /*errorCode*/)
{
     int i;
   if (bufEdit) {
         for(i=0; i<bufEdit->numChanges; i++)
             bibDealloc((void *) bufEdit->editParams);
         bibDealloc((void *) bufEdit);
   }
}


/*
*
* bibFlushBits
* bibGetBits
* bibShowBits
*    
*
* Parameters:
*    numberOfBits               the number of bits wanted (1..32)
*    buffer                     a pointer to a bit buffer structure
*    numberOfBitsGot            the number of bits actually got
*    syncStartIndex             the number of the first bit belonging to 
*                               a synchronization code (1 .. numberOfBits).
*                               Bits are numbered starting from 1 given to
*                               the most significant bit of the returned code.
*                               Thus, syncStartIndex - 1 most significant bits
*                               of the returned code are valid coding
*                               parameters (do not belong to a sync code).
*                               0 is returned if the returned code does not
*                               contain any part of a synchronization code.
*    bitErrorIndication         indication code for bit errors,
*                               see biterr.h for the possible values
*    errorCode                  error code
*
* Function:
*    The bibFlushBits function removes the next numberOfBits bits from
*    the buffer.
*
*    The bibGetBits function gets the next numberOfBits bits from the buffer.
*    The returned bits are removed.
*
*    The bibShowBits function gets the next numberOfBits bits from the
*    buffer. The returned bits are not removed from the buffer.
*
* Returns:
*   bibGetBits and bibShowBits:
*       the next numberOfBits bits / the rest of the buffer if the end of
*       the input file has been reached
*
* Error codes:
*    ERR_BIB_NOT_ENOUGH_DATA    if one tries to get more bits from the buffer
*                               than there is left
*
*/

void bibFlushBits(int numberOfBits, bibBuffer_t *buffer)
{
    bibAssert(buffer != NULL);
    bibAssert(numberOfBits > 0 && numberOfBits <= 32);
    
    /* Check if enough bits are available. */
    if (buffer->bitsLeft < (u_int32) numberOfBits) {
        goto flush_underflow;
    }
    buffer->bitsLeft -= numberOfBits;
    
    if ( buffer->bitIndex >= numberOfBits ) {
        /* All in the current byte, bits still left */
        buffer->bitIndex -= numberOfBits;
    }
    else if ( buffer->bitIndex == (numberOfBits-1) ) {
        /* Current byte used completely */
        buffer->bitIndex = 7;
        buffer->getIndex++;
        buffer->numBytesRead++;
    }
    else {
        /* Current byte plus then some */
        numberOfBits -= buffer->bitIndex + 1; /* this many bits after current byte */
        buffer->getIndex += ((numberOfBits>>3) + 1);
        buffer->numBytesRead += ((numberOfBits>>3) + 1);
        buffer->bitIndex = 7 - (numberOfBits&7); 
    }
    return;
flush_underflow:
    buffer->error = ERR_BIB_NOT_ENOUGH_DATA;
}



/*
*
* bibGetAlignedByte
*    
*
* A fast function to get one byte from byte-boundary.
*
*/
inline u_int8 bibGetAlignedByte(bibBuffer_t *buffer)   
    {
    return buffer->baseAddr[buffer->getIndex++];
    }


/*
*
* bibGetBits
*    
*
* See bibFlushBits.
*
*/

u_int32 bibGetBits(int numberOfBits, bibBuffer_t *buffer)   
{
    static const u_char
        msbMask[8] = {1, 3, 7, 15, 31, 63, 127, 255},
        lsbMask[9] = {255, 254, 252, 248, 240, 224, 192, 128, 0};
    u_char
        *startAddr;
    int32
        bitIndex;
    u_int32
        endShift;         /* the number of shifts after masking the last byte */
    u_int32
        numBytesFlushed;  /* the number of bytes flushed from the buffer */
    u_int32
        returnValue = 0;
    
    bibAssert(buffer != NULL);
    bibAssert(numberOfBits > 0 && numberOfBits <= 32);
    
    startAddr = buffer->baseAddr + buffer->getIndex;
    bitIndex = buffer->bitIndex;
    
    /* Check if enough bits are available. */
    if (buffer->bitsLeft < (u_int32) numberOfBits) {
        goto get_underflow;
    }
    
    buffer->bitsLeft -= numberOfBits;
    
    if ( bitIndex >= numberOfBits ) {
        /* All in the current byte, bits still left */
        endShift = bitIndex - numberOfBits + 1;
        buffer->bitIndex -= numberOfBits;
        return ((startAddr[0] & msbMask[bitIndex] & lsbMask[endShift]) >> endShift);
    }
    else if ( bitIndex == (numberOfBits-1) ) {
        /* Current byte used completely */
        buffer->bitIndex = 7;
        buffer->getIndex++;
        buffer->numBytesRead++;
        return startAddr[0] & msbMask[bitIndex];
    }
    else {
        /* Current byte plus then some */
        
        /* Remainder of this byte */
        returnValue = *(startAddr++) & msbMask[bitIndex];
        numberOfBits -= bitIndex + 1;
        numBytesFlushed = 1 + (numberOfBits >> 3);
        
        /* Get full bytes */
        while ( numberOfBits >= 8 ) {
            returnValue = (returnValue << 8) | *(startAddr++);
            numberOfBits -= 8;
        }
        
        /* Get bits from last byte */
        endShift = 8 - numberOfBits;
        returnValue = (returnValue << numberOfBits) | ((startAddr[0] & lsbMask[endShift]) >> endShift);
        /* (safe, since lsbMask[8]==0) */
        
        /* Update position in buffer */
        buffer->bitIndex = 7 - numberOfBits;
        buffer->getIndex += numBytesFlushed;
        buffer->numBytesRead += numBytesFlushed;
    }
    
    return returnValue;
get_underflow:
    buffer->error = ERR_BIB_NOT_ENOUGH_DATA;
    return 0;
}


/* {{-output"bibNumberOfFlushedBits.txt"}} */
/*
*
* bibNumberOfFlushedBits
*    
*
* Parameters:
*    buffer                     a pointer to a bit buffer structure
*
* Function:
*    The bibNumberOfFlushedBytes returns the number of bits which
*    are got (bibGetBits) or flushed (bibFlushBits) from the buffer since
*    the buffer was created
*
* Returns:
*    See above.
*
* Error codes:
*    None.
*
*/

u_int32 bibNumberOfFlushedBits(bibBuffer_t *buffer)
/* {{-output"bibNumberOfFlushedBits.txt"}} */
{
    bibAssert(buffer != NULL);
    
    return ((buffer->numBytesRead<<3) + (7-buffer->bitIndex));
}


/* {{-output"bibNumberOfFlushedBytes.txt"}} */
/*
*
* bibNumberOfFlushedBytes
*    
*
* Parameters:
*    buffer                     a pointer to a bit buffer structure
*
* Function:
*    The bibNumberOfFlushedBytes returns the number of whole bytes which
*    are got (bibGetBits) or flushed (bibFlushBits) from the buffer since
*    bibCreate was called.
*
* Returns:
*    See above.
*
* Error codes:
*    None.
*
*/

u_int32 bibNumberOfFlushedBytes(bibBuffer_t *buffer)
/* {{-output"bibNumberOfFlushedBytes.txt"}} */
{
    bibAssert(buffer != NULL);
    
    return buffer->numBytesRead;
}


/* {{-output"bibNumberOfRewBits.txt"}} */
/*
*
* bibNumberOfRewBits
*    
*
* Parameters:
*    buffer                     a pointer to a bit buffer structure
*
* Function:
*    The bibNumberOfRewBits returns the number of bits which can be
*    successfully rewinded.
*
* Returns:
*    See above.
*
* Error codes:
*    None.
*
*/

u_int32 bibNumberOfRewBits(bibBuffer_t *buffer)
/* {{-output"bibNumberOfRewBits.txt"}} */
{
    bibAssert(buffer != NULL);
    
    return ((buffer->numBytesRead<<3) + (7-buffer->bitIndex));
}


/* {{-output"bibRewindBits.txt"}} */
/*
*
* bibRewindBits
*    
*
* Parameters:
*    numberOfBits               the number of bits to rewind
*    buffer                     a pointer to a bit buffer structure
*    errorCode                  error code
*
* Function:
*    This function rewinds the pointers to the buffer
*    so that already flushed or got bits can be read
*    again.
*
* Returns:
*    Nothing.
*
* Error codes:
*    ERR_BIB_CANNOT_REWIND      if numberOfBits is larger than which is
*                               possible to get (see also bibNumberOfRewBits).
*
*    ERR_BIB_BUFLIST            if there was a fatal error when handling
*                               buffer lists
*
*/

void bibRewindBits(u_int32 numberOfBits, bibBuffer_t *buffer, int16 *errorCode)
/* {{-output"bibRewindBits.txt"}} */
{
    bibAssert(buffer != NULL);
    
    /* All bits to rewind are in the latest byte */
    if (numberOfBits <= (u_int32) (7 - buffer->bitIndex)) {
        buffer->bitIndex += numberOfBits;
    }
    /* Bits to rewind are within several bytes */
    else {
        u_int32
            numBitsWithoutFirstByte = numberOfBits - (7 - buffer->bitIndex),
            numWholeBytes = (numBitsWithoutFirstByte>>3),
            numBitsInLastByte = numBitsWithoutFirstByte - (numWholeBytes<<3);
        
        if (numBitsInLastByte) {
            if (buffer->getIndex >= numWholeBytes + 1) {
                buffer->getIndex -= numWholeBytes + 1;
                buffer->bitIndex = numBitsInLastByte - 1;
                buffer->numBytesRead -= numWholeBytes + 1;
            }
            else {
                *errorCode = ERR_BIB_CANNOT_REWIND;
                deb("bibRewindBits: ERROR - cannot rewind.\n");
            }
        }
        else {
            if (buffer->getIndex >= numWholeBytes) {
                buffer->getIndex -= numWholeBytes;
                buffer->bitIndex = 7;
                buffer->numBytesRead -= numWholeBytes;
            }
            else {
                *errorCode = ERR_BIB_CANNOT_REWIND;
                deb("bibRewindBits: ERROR - cannot rewind.\n");
            }
        }
    }
    buffer->bitsLeft += numberOfBits;
}


/* {{-output"bibShowBits.txt"}} */
/*
*
* bibShowBits
*    
*
* See bibFlushBits.
*
*/

u_int32 bibShowBits(int numberOfBits, bibBuffer_t *buffer)  
/* {{-output"bibShowBits.txt"}} */
{
    static const u_char
        msbMask[8] = {1, 3, 7, 15, 31, 63, 127, 255},
        lsbMask[9] = {255, 254, 252, 248, 240, 224, 192, 128, 0};
    u_char
        *startAddr;
    int32
        bitIndex;
    u_int32
        endShift;         /* the number of shifts after masking the last byte */
    u_int32
        returnValue = 0;
    
    bibAssert(buffer != NULL);
    bibAssert(numberOfBits > 0 && numberOfBits <= 32);
    
    startAddr = buffer->baseAddr + buffer->getIndex;
    bitIndex = buffer->bitIndex;
    
    /* Check if enough bits are available. */
    if (buffer->bitsLeft < (u_int32) numberOfBits) {
        goto show_underflow;
    }
    
    if ( bitIndex >= numberOfBits ) {
        /* All in the current byte, bits still left */
        endShift = bitIndex - numberOfBits + 1;
        return ((startAddr[0] & msbMask[bitIndex] & lsbMask[endShift]) >> endShift);
    }
    else if ( bitIndex == (numberOfBits-1) ) {
        /* Current byte used completely */
        return startAddr[0] & msbMask[bitIndex];
    }
    else {
        /* Current byte plus then some */
        
        /* Remainder of this byte */
        returnValue = *(startAddr++) & msbMask[bitIndex];
        numberOfBits -= bitIndex + 1;
        
        /* Get full bytes */
        while ( numberOfBits >= 8 )
        {
            returnValue = (returnValue << 8) | *(startAddr++);
            numberOfBits -= 8;
        }
        
        /* Get bits from last byte */
        endShift = 8 - numberOfBits;
        returnValue = (returnValue << numberOfBits) | ((startAddr[0] & lsbMask[endShift]) >> endShift);
        /* (safe, since lsbMask[8]==0) */
    }
    
    return returnValue;
show_underflow:
    buffer->error = ERR_BIB_NOT_ENOUGH_DATA;
    return 0;
}


/*
* Local functions
*/

/*
*
* bibInitialize
*    
*
* Parameters:
*    bibBuffer                  input bit buffer instance
*    srcBuffer                  buffer containing the data
*    srcBufferLength            the length of the buffer
*    errorCode                  error code
*
* Function:
*    This function initializes the values of the bibBuffer structure.
*
* Returns:
*    Nothing.
*
* Error codes:
*    ERR_BIB_BUFLIST            if the internal buffer list has been corrupted
*
*/

static void bibInitialize(
                            bibBuffer_t *bibBuffer,
                            void *srcBuffer,
                            unsigned srcBufferLength,
                            int16 */*errorCode*/)
{
    bibBuffer->baseAddr = (u_char *) srcBuffer;
    bibBuffer->size = srcBufferLength;
    bibBuffer->getIndex = 0;
    bibBuffer->bitIndex = 7;
    bibBuffer->bitsLeft = (u_int32) (srcBufferLength<<3);
    bibBuffer->numBytesRead = 0;
    bibBuffer->error = 0;
    
    
}



/*
*
* CopyStream
*    
*
* Function to copy stream from SrcBuffer to DestBuffer based on settings in bufEdit and ByteStart & BitStart
*    
*    
*/

void CopyStream(bibBuffer_t *SrcBuffer,bibBuffer_t *DestBuffer,bibBufferEdit_t *bufEdit, 
                                int ByteStart, int BitStart)
{
    int32 temp;
    unsigned tgetIndex;  
    int tbitIndex;
    u_int32 tbitsLeft;
    u_int32 tnumBytesRead;
    int tByteStart; 
    int tBitStart;  
    
    bibEditParams_t *edParam;
    
    //Add assertions and checks here !!
    bibAssert(SrcBuffer->baseAddr);
    bibAssert(DestBuffer->baseAddr);
    bibAssert(bufEdit);
    
    //Save the params of SrcBuffer to recover them later:
    tgetIndex=SrcBuffer->getIndex;
    tbitIndex=SrcBuffer->bitIndex;
    tbitsLeft=SrcBuffer->bitsLeft;
    tnumBytesRead=SrcBuffer->numBytesRead;
    
    
    
    // check to see if we need to change some header parameter
    if(bufEdit->copyMode == CopyWithEdit/*CopyWithEdit*/)
    {
        bibAssert(bufEdit->editParams);
        edParam = &(bufEdit->editParams[0]); 
        
        // check if the editing position is in the current range of bit data
        temp=((SrcBuffer->getIndex<<3) + (7-SrcBuffer->bitIndex))-
            ((edParam->StartByteIndex<<3) + 7-(edParam->StartBitIndex));
        if (temp>=0)    // yes, it is
        {
            // copy upto the editing point
            CopyBuffer(SrcBuffer, DestBuffer, ByteStart, BitStart, edParam->StartByteIndex, 
                edParam->StartBitIndex);
            
            CopyBufferEdit(SrcBuffer, DestBuffer, &(bufEdit->editParams[0]));
            
            // store new starting copy position 
            tByteStart = SrcBuffer->getIndex; 
            tBitStart  = SrcBuffer->bitIndex; 
            // restore original stop copy position 
            SrcBuffer->getIndex=tgetIndex;
            SrcBuffer->bitIndex=tbitIndex;
            SrcBuffer->bitsLeft=tbitsLeft;
            SrcBuffer->numBytesRead=tnumBytesRead;
            
            CopyBuffer(SrcBuffer, DestBuffer, tByteStart, tBitStart, tgetIndex, tbitIndex);

        }
        else                    // no
        {
            // put panic here !
            return;
        }
    }
    else if (bufEdit->copyMode == CopyWhole/*CopyWhole*/)
    {
        CopyBuffer(SrcBuffer, DestBuffer, ByteStart, BitStart, 
            SrcBuffer->getIndex, SrcBuffer->bitIndex);
    }
    else if (bufEdit->copyMode == EditOnly /*EditOnly*/)
    {
        CopyBufferEdit(SrcBuffer, DestBuffer, &(bufEdit->editParams[0]));
    }
    else if(bufEdit->copyMode == CopyNone/*CopyNone*/)
    {
        return; 
    }

    
  //4- Retrieve the original Srcbuffer params.
    // Using getbits it should be equal to where we started
    SrcBuffer->getIndex=tgetIndex;
    SrcBuffer->bitIndex=tbitIndex;
    SrcBuffer->bitsLeft=tbitsLeft;
    SrcBuffer->numBytesRead=tnumBytesRead;
    
    //5- Update the destbuffer statistics:
    DestBuffer->getIndex=DestBuffer->numBytesRead;

    DestBuffer->bitsLeft -= (DestBuffer->getIndex<<3);

}

/*
*
* CopyBuffer
*    
*
* Function to copy data from SrcBuffer to DestBuffer from ByteStart & BitStart to ByteEnd & BitEnd
*    
*    
*/

void CopyBuffer(bibBuffer_t *SrcBuffer,bibBuffer_t *DestBuffer,
                                int ByteStart,int BitStart, int ByteEnd, int BitEnd)
{
    u_char *DestStartAddr;
    u_int32 i;
    u_int32 temp;
    u_int32 BitsToRewind;
    u_int32 BitsToCopy;
    u_int32 BytesToCopy;
    u_int32 Bytes4ToCopy;
    u_int32 BitsRemaining;
    int16 errorCode;
    u_int32 bitshift;
    u_int32 bitstoget;
    static const u_char msbMask[8] = {1, 3, 7, 15, 31, 63, 127, 255};
    
    DestStartAddr = DestBuffer->baseAddr + DestBuffer->getIndex;
    
    //Rewind the src buffer to the start address(ByteStart,BitStart)
    BitsToRewind=((SrcBuffer->getIndex<<3) + (7-SrcBuffer->bitIndex))-
        ((ByteStart<<3) + (7-BitStart));
    bibRewindBits(BitsToRewind,SrcBuffer, &errorCode);
    
    // evaluate the number of bits to copy
    BitsToCopy = ((ByteEnd<<3) + (7-BitEnd))-((ByteStart<<3) + (7-BitStart));
    if (BitsToCopy<=0)
        return; 
    else if (BitsToCopy > BitsToRewind) 
        BitsToCopy = BitsToRewind;      // or else provide a panic here !!!
    
    //1- Fill the remaining of the byte in destination:
    bitshift=0;
    bitstoget=0;
    if(DestBuffer->bitIndex!=7)
        {
        bitshift = DestBuffer->bitIndex+1;
        bitstoget = ((BitsToCopy < bitshift) ? BitsToCopy : bitshift); 
        temp = bibGetBits(bitstoget,SrcBuffer);
        
        // update statistics to take care of bit addition or byte completion 
        if (BitsToCopy < bitshift)
        {
            // bits added but byte not completed
            *(DestStartAddr)=(unsigned char)((((*DestStartAddr)>>bitshift)<<bitshift) |
                ((temp << (bitshift-BitsToCopy)  ) & msbMask[bitshift-1]));
            DestBuffer->bitIndex -= BitsToCopy;
            bibAssert(DestBuffer->bitIndex >= 0)
        }
        else
        {
            // byte completed
            *(DestStartAddr)=(unsigned char)((((*DestStartAddr)>>bitshift)<<(bitshift)) | 
                (temp & msbMask[bitshift-1]));
            DestStartAddr+=1; 
            DestBuffer->numBytesRead++;
            DestBuffer->bitIndex=7;
        }
    }
    
    //2- Extract all bytes (in 8 bits) from src to destination.
    //Checks for BytesToCopy. 
    BytesToCopy=(BitsToCopy-bitstoget)>>3;
    if ( BytesToCopy > 0 )
    {

        if ( SrcBuffer->bitIndex == 7 )
        {
            // we can copy the data from src in full bytes, utilize faster inline method 
            // and try to utilize pipelining in the for-loop
            // truncate it to 4-byte-boundary
            Bytes4ToCopy = BytesToCopy>>2;
            for(i=0; i<Bytes4ToCopy; i++)
            {
                *(DestStartAddr++) = bibGetAlignedByte(SrcBuffer);
                *(DestStartAddr++) = bibGetAlignedByte(SrcBuffer);
                *(DestStartAddr++) = bibGetAlignedByte(SrcBuffer);
                *(DestStartAddr++) = bibGetAlignedByte(SrcBuffer);
            }
            i <<= 2;
            if ( BytesToCopy > i )
            {
                // copy the leftovers
                for(;i<BytesToCopy;i++)
                {
                    *(DestStartAddr++) = bibGetAlignedByte(SrcBuffer);
                }
            }
            SrcBuffer->numBytesRead+=i;
        }
        else
        {
            for(i=0;i<BytesToCopy;i++)
            {
                *(DestStartAddr++) = (unsigned char)bibGetBits(8,SrcBuffer);
            }
            
        }
    }
    DestBuffer->numBytesRead+=BytesToCopy;
    
    //3- Fill the last byte:
    BitsRemaining=((BitsToCopy-bitstoget))%8;
    if(BitsRemaining!=0)
    {
        temp = bibGetBits(BitsRemaining,SrcBuffer);
        *(DestStartAddr++)=(u_char)(temp<<(8-BitsRemaining)); 
        DestBuffer->bitIndex=7-BitsRemaining;
    }
    
    //5- Update the destbuffer statistics:
    DestBuffer->getIndex=DestBuffer->numBytesRead;
}

/*
*
* CopyBufferEdit
*    
*
* Function to copy data with editing from SrcBuffer to DestBuffer with settings in edParam
*    
*    
*/

void CopyBufferEdit(bibBuffer_t *SrcBuffer, bibBuffer_t *DestBuffer, 
                                        bibEditParams_t *edParam, int updateSrcBufferStats)
{
    u_char *DestStartAddr;
    u_int32 i;
    u_int32 temp;
    u_int32 BitsToSkip;
    u_int32 BitsToEdit;
    u_int32 BytesToSkip;
    u_int32 BitsRemaining;
    u_int32 BytesToEdit;
    unsigned bitshift;
    unsigned bitstoget;
    unsigned bitstomove;
    u_int32 StartBitPosition; 
    static const u_char msbMask[8] = {1, 3, 7, 15, 31, 63, 127, 255};
    
    DestStartAddr = DestBuffer->baseAddr + DestBuffer->getIndex;
    
    // evaluate the number of bits to copy
    BitsToEdit = edParam->newNumBits; 
    StartBitPosition = edParam->newNumBits-1;
    
    //1- Fill the remaining of the byte in destination:
    bitshift=0;
    bitstoget=0;
    if(DestBuffer->bitIndex!=7)
    {
        bitshift=DestBuffer->bitIndex+1;
        bitstoget = ((BitsToEdit < bitshift) ? BitsToEdit : bitshift); 
        
        temp = bibGetBitsFromWord(edParam->newValue, bitstoget, &StartBitPosition, 
            edParam->newNumBits); 
        
        // update statistics to take care of bit addition or byte completion 
        if (BitsToEdit < bitshift)
        {
            // bits added but byte not completed
            *(DestStartAddr)=(unsigned char)((((*DestStartAddr)>>bitshift)<<(bitshift)) |
                ((temp << (bitshift-BitsToEdit)  ) & msbMask[bitshift-1]));
            DestBuffer->bitIndex -= BitsToEdit;
            bibAssert(DestBuffer->bitIndex >= 0)
        }
        else
        {
            // byte completed
            *(DestStartAddr)=(unsigned char)((((*DestStartAddr)>>bitshift)<<(bitshift)) | 
                (temp & msbMask[bitshift-1]));
            DestStartAddr++; 
            DestBuffer->numBytesRead++;
            DestBuffer->bitIndex=7;
        }
    }
    
    
    //2- Extract all bytes (in 8 bits) from src to destination.
    //Checks for BytesToCopy
    BytesToEdit=(BitsToEdit-bitstoget)>>3;
    for(i=0;i<BytesToEdit;i++)
    {
        *(DestStartAddr++)=(unsigned char)bibGetBitsFromWord(edParam->newValue, 8, &StartBitPosition, edParam->newNumBits);
    }
    DestBuffer->numBytesRead+=BytesToEdit;
    
    //3- Fill the last byte:
    BitsRemaining=((BitsToEdit-bitstoget))%8;
    if(BitsRemaining!=0)
    {
        temp = bibGetBitsFromWord(edParam->newValue, BitsRemaining, &StartBitPosition, 
            edParam->newNumBits); 
        *(DestStartAddr++)=(u_char)(temp<<(8-BitsRemaining)); 
        DestBuffer->bitIndex=7-BitsRemaining;
    }
    
    //5- Update the destbuffer statistics:
    DestBuffer->getIndex=DestBuffer->numBytesRead;
    
    
    // update the src buffer statistics to reflect skipping of the value 
    if(updateSrcBufferStats)
    {
        BitsToSkip = edParam->curNumBits; 
        bitshift=0;
        bitstomove=0;
        if (SrcBuffer->bitIndex!=7)
        {
            bitshift=SrcBuffer->bitIndex+1;
            bitstomove = ((BitsToSkip < bitshift) ? BitsToSkip : bitshift); 
            // update statistics to take care of bit addition or byte completion 
            if (BitsToSkip < bitshift)
            {
                // bits skipped but byte not completed
                SrcBuffer->bitIndex -= bitstomove;
                bibAssert(SrcBuffer->bitIndex >= 0)
            }
            else
            {
                // byte completed
                SrcBuffer->numBytesRead++;
                SrcBuffer->bitIndex=7;
            }
        }
        // full bytes to skip
        BytesToSkip=(BitsToSkip-bitstomove)>>3;
        SrcBuffer->numBytesRead+=BytesToSkip;
        
        // skip the remaining bits
        BitsRemaining=((BitsToSkip-bitstomove))%8;
        if(BitsRemaining!=0)
        {
            SrcBuffer->bitIndex=7-BitsRemaining;
        }
        SrcBuffer->bitsLeft -= BitsToSkip;
        SrcBuffer->getIndex=SrcBuffer->numBytesRead;
    }
}

void ResetH263IntraDcUV(bibBuffer_t *DestBuffer, int uValue, int vValue)
{
    bibEditParams_t edParam;

    edParam.curNumBits = edParam.newNumBits = 8;
    edParam.StartByteIndex = edParam.StartBitIndex = 0; // used for source buffer only 

  // u
    edParam.newValue = uValue; 
    CopyBufferEdit((bibBuffer_t*)NULL, DestBuffer, &edParam, 0); 
    // v
    edParam.newValue = vValue; 
    CopyBufferEdit((bibBuffer_t*)NULL, DestBuffer, &edParam, 0); 
}

void ResetMPEG4IntraDcUV(bibBuffer_t *DestBuffer, int IntraDC_size)
{
    int i;
    bibEditParams_t edParam;
    const int DctDcSizeChrominanceNumBits[13] = { 2, 2, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12 }; 

    // u,v
    for(i=0; i<2; i++)
    {
        // change dct dc size chrominance - IntraDC for U.V is 0 (codeword '11')
        edParam.curNumBits = DctDcSizeChrominanceNumBits[IntraDC_size]; 
        edParam.newNumBits = 2;  
        edParam.StartByteIndex = edParam.StartBitIndex = 0; // used or source buffer only 
        edParam.newValue = 3; 
        CopyBufferEdit((bibBuffer_t*)NULL, DestBuffer, &edParam, 0);    
    }
}

// assume SrcValue is max 32 bits 
u_int32 bibGetBitsFromWord(u_int32 SrcValue, u_int32 getBits, u_int32 *StartBit, 
                                                     u_int32 MaxNumBits)
{
    int val;
    u_int32 bitshift; 
    static const u_int32 mask[32] = 
            {0x00000001, 0x00000003, 0x00000007, 0x0000000f, 0x0000001f, 0x0000003f, 0x0000007f, 0x000000ff, 
             0x000001ff, 0x000003ff, 0x000007ff, 0x00000fff, 0x00001fff, 0x00003fff, 0x00007fff, 0x0000ffff,
             0x0001ffff, 0x0003ffff, 0x0007ffff, 0x000fffff, 0x001fffff, 0x003fffff, 0x007fffff, 0x00ffffff, 
             0x01ffffff, 0x03ffffff, 0x07ffffff, 0x0fffffff, 0x1fffffff, 0x3fffffff, 0x7fffffff, 0xffffffff};

    bibAssert(MaxNumBits <= 32);
    bibAssert(*StartBit < MaxNumBits);
    bibAssert(getBits-1 <= *StartBit);
    bibAssert(getBits > 0);
    
    bitshift = *StartBit - getBits + 1; 
    val = (SrcValue>>bitshift) & mask[getBits-1]; 
    if ( getBits > *StartBit )
        {
        // taking the last bits of the word; *StartBit is uint32, so it goes to max uint32 unless this special handling
        // other cases asserted already above
        *StartBit = 31;
        }
    else
        {
        *StartBit -= getBits; 
        }

    return val;
}

void bibForwardBits(u_int32 numberOfBits, bibBuffer_t *buffer)
{
    u_int32 BitsToForward;
    u_int32 BytesToForward;
    u_int32 BitsRemaining;
    unsigned bitshift;
    unsigned bitstomove;
        
    BitsToForward = numberOfBits; 
    bitshift=0;
    bitstomove=0;
        
    bibAssert(buffer != NULL);
        
        // complete the byte
    if (buffer->bitIndex!=7)
    {
        bitshift=buffer->bitIndex+1;
        bitstomove = ((BitsToForward < bitshift) ? BitsToForward : bitshift); 
        // update statistics to take care of bit addition or byte completion 
        if (BitsToForward < bitshift)
        {
            // bits skipped but byte not completed
            buffer->bitIndex -= bitstomove;
            bibAssert(buffer->bitIndex >= 0)
        }
        else
        {
            // byte completed
            buffer->numBytesRead++;
            buffer->bitIndex=7;
        }
    }
        // full bytes to skip
    BytesToForward=(BitsToForward-bitstomove)>>3;
    buffer->numBytesRead+=BytesToForward;
        
        // skip the remaining bits
    BitsRemaining=((BitsToForward-bitstomove))%8;
    if(BitsRemaining!=0)
    {
        buffer->bitIndex=7-BitsRemaining;
    }
    buffer->bitsLeft -= BitsToForward;
    buffer->getIndex=buffer->numBytesRead;
}

void bibStuffBits(bibBuffer_t *buffer)
{
    // the extra bits are already set to zero 
    bibAssert(buffer->baseAddr);
    if(buffer->bitIndex!=7)
    {
        buffer->bitIndex=7;
        buffer->getIndex++;
        buffer->numBytesRead++;
    }
}




// End of File
