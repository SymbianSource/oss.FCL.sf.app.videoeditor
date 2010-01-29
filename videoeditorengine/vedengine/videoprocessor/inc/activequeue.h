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
* Data buffering and queuing support definitions, class CActiveQueue.
*
*/


#ifndef     __ACTIVEQUEUE_H__
#define     __ACTIVEQUEUE_H__


//  INCLUDES

#ifndef __E32BASE_H__
#include <e32base.h>
#endif


//  FORWARD DECLARATIONS

class CDataProcessor;
class CActiveQueue;


//#define BLOCK_GUARD_AREA


//  CLASS DEFINITIONS

// Queue block class
class TActiveQueueBlock : public TPtr8
{
    TActiveQueueBlock(TUint8 *aBuf, TInt aMaxLength) : TPtr8(aBuf, aMaxLength) {};
protected:
    friend class CActiveQueue;
    TActiveQueueBlock *iNextBlock; // next block in the allocated block list
    TActiveQueueBlock *iPrevBlock; // prev. block in the allocated block list
    TActiveQueueBlock *iNextList; // next block in the data queue / free list
};

// The queue class
class CActiveQueue : public CBase
{
public: // constants
    enum TErrorCode
    {
        EInternalAssertionFailure = -1000,
        ENoWriter = -1001, 
        ENoReader = -1002,
        EWriteAfterStreamEnd = -1003
    };

public: // interface functions
    // Constructors and destructor
    CActiveQueue(TUint aNumberOfBlocks, TUint aBlockLength);
    ~CActiveQueue();
    void ConstructL();

    // Set/remove reader
    void SetReader(CDataProcessor *aReader, TAny *aUserPointer);
    void RemoveReader();

    // Set/remove writer
    void SetWriter(CDataProcessor *aWriter, TAny *aUserPointer);
    void RemoveWriter();

    void ResetStreamEnd();
    
    // Get the number of free blocks available for new data
    TUint NumFreeBlocks();

    // Get the number of blocks with data queued
    TUint NumDataBlocks();

    // Get a free block for writing data into the queue (writer)
    TPtr8 *GetFreeBlockL(TUint aBlockLength);

    // Add a data block to the queue (writer)
    void WriteBlock(TPtr8 *aBlock);

    // Read a data block from the queue (reader)
    TPtr8 *ReadBlock();

    // Return a read block back to the empty block list (reader)
    void ReturnBlock(TPtr8 *aBlock);

    // Notify that the stream has ended (writer)
    void WriteStreamEnd();

    // Check if the writer has notified that the stream has ended (reader)
    TBool StreamEnded();

    // Testing aid: get total number of blocks
    TUint NumBlocks() { return iNumBlocks; };


private: // Internal methods
    TActiveQueueBlock *AllocateBlockL();
    void FreeBlockL(TActiveQueueBlock *aBlock);
    
private: // Data
    TUint iNewBlockLength; // length of new blocks that are allocated
    TUint iInitialBlocks; // initial number of blocks to allocate
    
    CDataProcessor *iReader; // the reader
    TAny *iReaderUserPointer;
    CDataProcessor *iWriter; // the writer
    TAny *iWriterUserPointer;

    TActiveQueueBlock *iBlocks; // the list of all allocated blocks
    TUint iNumBlocks; // total number of blocks
    TActiveQueueBlock *iFreeList; // the free block list
    TUint iNumFreeBlocks; // number of free blocks
    TActiveQueueBlock *iDataQueueHead; // the data queue head (where blocks are
                                       // read from)
    TActiveQueueBlock *iDataQueueTail; // the data queue tail (where blocks are
                                       // written to
    TUint iNumDataBlocks; // number of data blocks

    TBool iStreamEnd; // has the stream ended?
};


#endif      //  __ACTIVEQUEUE_H__
            
// End of File
