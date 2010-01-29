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


//  EXTERNAL RESOURCES  


//  Include Files  

#include "dataprocessor.h"
#include "activequeue.h"


//  MEMBER FUNCTIONS


//=============================================================================


/*
-----------------------------------------------------------------------------

    CActiveQueue

    CActiveQueue()

    Standard C++ constructor

-----------------------------------------------------------------------------
*/

CActiveQueue::CActiveQueue(TUint aNumberOfBlocks, TUint aBlockLength)
{
    // Remember the number of blocks and initial new block length
    iInitialBlocks = aNumberOfBlocks;
    iNewBlockLength = aBlockLength;   
    iStreamEnd = EFalse;
}



/*
-----------------------------------------------------------------------------

    CActiveQueue

    ~CActiveQueue()

    Standard C++ destructor

-----------------------------------------------------------------------------
*/

CActiveQueue::~CActiveQueue()
{
    // Deallocate all blocks:
    while ( iBlocks )
    {
        TRAPD( error, FreeBlockL(iBlocks) );
        if (error != KErrNone) { }
    }

    __ASSERT_DEBUG(iNumBlocks == 0,
                   User::Panic(_L("CActiveQueue"), EInternalAssertionFailure));
}



/*
-----------------------------------------------------------------------------

    CActiveQueue

    ConstructL()

    Standard Symbian OS second-phase constructor, prepares the object for use

-----------------------------------------------------------------------------
*/

void CActiveQueue::ConstructL()
{
    // Allocate initial blocks
    while ( iInitialBlocks-- )
    {
        // Get block
        TActiveQueueBlock *block = AllocateBlockL();

        // Add it to the free block list
        block->iNextList = iFreeList;
        iFreeList = block;
        iNumFreeBlocks++;
    }
}



/*
-----------------------------------------------------------------------------

    CActiveQueue

    SetReader()

    Sets a reader for the queue

-----------------------------------------------------------------------------
*/

void CActiveQueue::SetReader(CDataProcessor *aReader, TAny *aUserPointer)
{
    iReader = aReader;
    iReaderUserPointer = aUserPointer;
}



/*
-----------------------------------------------------------------------------

    CActiveQueue

    RemoveReader()

    Removes a reader from the queue

-----------------------------------------------------------------------------
*/

void CActiveQueue::RemoveReader()
{
    __ASSERT_DEBUG(iReader, User::Panic(_L("CActiveQueue"), ENoReader));
    iReader = 0;
}



/*
-----------------------------------------------------------------------------

    CActiveQueue

    SetWriter()

    Sets a writer for the queue

-----------------------------------------------------------------------------
*/

void CActiveQueue::SetWriter(CDataProcessor *aWriter, TAny *aUserPointer)
{
    iWriter = aWriter;
    iWriterUserPointer = aUserPointer;
}



/*
-----------------------------------------------------------------------------

    CActiveQueue

    RemoveWriter()

    Removes a writer from the queue

-----------------------------------------------------------------------------
*/

void CActiveQueue::RemoveWriter()
{
    __ASSERT_DEBUG(iWriter, User::Panic(_L("CActiveQueue"), ENoWriter));
    iWriter = 0;
}

/*
-----------------------------------------------------------------------------

    CActiveQueue

    ResetStreamEnd()

    Reset the status of the queue

-----------------------------------------------------------------------------
*/

void CActiveQueue::ResetStreamEnd()
{
    iStreamEnd = EFalse;
    // we should not have any blocks in full queue if we have reached stream end
}



/*
-----------------------------------------------------------------------------

    CActiveQueue

    NumFreeBlocks()

    Get the number of free blocks available for new data

-----------------------------------------------------------------------------
*/

TUint CActiveQueue::NumFreeBlocks()
{
    return iNumFreeBlocks;
}



/*
-----------------------------------------------------------------------------

    CActiveQueue

    NumDataBlocks()

    Get the number of blocks with data queued

-----------------------------------------------------------------------------
*/

TUint CActiveQueue::NumDataBlocks()
{
    return iNumDataBlocks;
}



/*
-----------------------------------------------------------------------------

    CActiveQueue

    GetFreeBlockL()

    Get a free block for writing data into the queue (writer)

-----------------------------------------------------------------------------
*/

TPtr8 *CActiveQueue::GetFreeBlockL(TUint aBlockLength)
{
    __ASSERT_DEBUG(iWriter, User::Panic(_L("CActiveQueue"), ENoWriter));
    TActiveQueueBlock *block = 0;
    
    // If the requested block size is larger than the currently used length
    // for new blocks, use the new size for all new blocks
    if ( aBlockLength > iNewBlockLength )
        iNewBlockLength = aBlockLength;

    // Do we have free blocks?
    if ( iNumFreeBlocks )
    {
        // Yes, get a block from the queue:
        __ASSERT_DEBUG(iFreeList != 0,
                       User::Panic(_L("CActiveQueue"), EInternalAssertionFailure));
        block = iFreeList;
        iFreeList = block->iNextList;
        iNumFreeBlocks--;

        // If the block isn't large enough, discard it so that we'll allocate a
        // new one. Don't discard more than one block to keep the number of
        // blocks allocated constant.
        if ( block->MaxLength() < (TInt) aBlockLength )
        {
            FreeBlockL(block);
            block = 0;
        }
    }

    // If we didn't get a suitable block, allocate a new one
    if ( !block )
        block = AllocateBlockL();

    __ASSERT_DEBUG(block->MaxLength() >= (TInt) aBlockLength,
                   User::Panic(_L("CActiveQueue"), EInternalAssertionFailure));

    block->SetLength(0);

    return block;
}



/*
-----------------------------------------------------------------------------

    CActiveQueue

    WriteBlock()

    Add a data block to the queue (writer)

-----------------------------------------------------------------------------
*/

void CActiveQueue::WriteBlock(TPtr8 *aBlock)
{
    __ASSERT_DEBUG(iWriter, User::Panic(_L("CActiveQueue"), ENoWriter));
    __ASSERT_DEBUG(!iStreamEnd,
                   User::Panic(_L("CActiveQueue"), EWriteAfterStreamEnd));
                   
    // The block is really a TActiveQueueBlock
    TActiveQueueBlock *block = (TActiveQueueBlock*) aBlock;


    // Add the block to the queue:
    if ( iDataQueueTail )
    {
        // The queue is not empty
        __ASSERT_DEBUG(iDataQueueHead && iNumDataBlocks,
                       User::Panic(_L("CActiveQueue"), EInternalAssertionFailure));
        iDataQueueTail->iNextList = block;
        iDataQueueTail = block;
        block->iNextList = 0;
    }
    else
    {
        // The queue is empty -> this will be the first block
        __ASSERT_DEBUG((!iDataQueueHead) && (!iNumDataBlocks),
                       User::Panic(_L("CActiveQueue"), EInternalAssertionFailure));
        iDataQueueHead = block;
        iDataQueueTail = block;
        block->iNextList = 0;
    }
    iNumDataBlocks++;

    // If we have a reader, notify it about the new data
    if ( iReader )
        iReader->InputDataAvailable(iReaderUserPointer);
}



/*
-----------------------------------------------------------------------------

    CActiveQueue

    ReadBlock()

    Read a data block from the queue (reader)

-----------------------------------------------------------------------------
*/

TPtr8 *CActiveQueue::ReadBlock()
{
    __ASSERT_DEBUG(iReader, User::Panic(_L("CActiveQueue"), ENoReader));
    __ASSERT_DEBUG(((iNumDataBlocks && iDataQueueHead) ||
                    ((!iNumDataBlocks) && (!iDataQueueHead))),
                   User::Panic(_L("CActiveQueue"), EInternalAssertionFailure));

    // If we don't have a block, return NULL
    if ( !iNumDataBlocks )
        return 0;

    // Get the block from the queue head
    TActiveQueueBlock *block = iDataQueueHead;
    iDataQueueHead = block->iNextList;
    iNumDataBlocks--;
    if ( !iNumDataBlocks )
    {
        // It was the only block in the queue
        __ASSERT_DEBUG((!iDataQueueHead) && (iDataQueueTail == block),
                       User::Panic(_L("CActiveQueue"), EInternalAssertionFailure));
        iDataQueueTail = 0;                       
    }
    __ASSERT_DEBUG((iDataQueueHead != block) && (iDataQueueTail != block),
                   User::Panic(_L("CActiveQueue"), EInternalAssertionFailure));

    // If it was the last block and the stream end has been signaled, notify
    // the reader
    if ( iStreamEnd && (!iNumDataBlocks) )
        iReader->StreamEndReached(iReaderUserPointer);

    // Return the block
    return ((TPtr8*) block);
}


/*
-----------------------------------------------------------------------------

    CActiveQueue

    ReturnBlock()

    Return a read block back to the empty block list (reader)

-----------------------------------------------------------------------------
*/

void CActiveQueue::ReturnBlock(TPtr8 *aBlock)
{
    __ASSERT_DEBUG(iReader, User::Panic(_L("CActiveQueue"), ENoReader));
                   
    // The block is really a TActiveQueueBlock
    TActiveQueueBlock *block = (TActiveQueueBlock*) aBlock;


    // Add it to the free list:
    __ASSERT_DEBUG((((!iNumFreeBlocks) && (!iFreeList)) ||
                    (iNumFreeBlocks && iFreeList)),
                   User::Panic(_L("CActiveQueue"), EInternalAssertionFailure));

    if ( block->MaxLength() == 0 )
    {
        block->MaxLength();
    }

    block->iNextList = iFreeList;
    iFreeList = block;
    iNumFreeBlocks++;

    // If we have a writer, notify it about the empty block
    if ( iWriter )
        iWriter->OutputSpaceAvailable(iWriterUserPointer);
}



/*
-----------------------------------------------------------------------------

    CActiveQueue

    WriteStreamEnd()

    Notify that the stream has ended (writer)

-----------------------------------------------------------------------------
*/

void CActiveQueue::WriteStreamEnd()
{
    __ASSERT_DEBUG(iWriter, User::Panic(_L("CActiveQueue"), ENoWriter));

    // Mark that the stream has ended
    iStreamEnd = ETrue;

    // If we have a reader and there are no more blocks in the queue, signal
    // the reader
    if ( iReader && (!iNumDataBlocks) )
        iReader->StreamEndReached(iReaderUserPointer);
}



/*
-----------------------------------------------------------------------------

    CActiveQueue

    StreamEnded()

    Check if the writer has notified that the stream has ended (reader)

-----------------------------------------------------------------------------
*/

TBool CActiveQueue::StreamEnded()
{
    return iStreamEnd;
}



/*
-----------------------------------------------------------------------------

    CActiveQueue

    AllocateBlockL()

    Allocates a new data block of size iNewBlockLengh

-----------------------------------------------------------------------------
*/

TActiveQueueBlock *CActiveQueue::AllocateBlockL()
{
    // Allocate the data area for the block
    TUint8 *data = (TUint8*) User::AllocLC(iNewBlockLength);    

    // Allocate the block
    TActiveQueueBlock *block = new (ELeave) TActiveQueueBlock(data,
                                                              iNewBlockLength);

    CleanupStack::Pop(data);

    // Add the block to the list of all blocks:
    if ( iBlocks )
    {
        block->iNextBlock = iBlocks;
        block->iPrevBlock = 0;
        iBlocks->iPrevBlock = block;
        iBlocks = block;
    }
    else
    {
        block->iNextBlock = 0;
        block->iPrevBlock = 0;
        iBlocks = block;
    }

    __ASSERT_DEBUG((iBlocks &&
                    ((iBlocks->iNextBlock == 0) ||
                     (iBlocks->iNextBlock != iBlocks))),
                   User::Panic(_L("CActiveQueue"), EInternalAssertionFailure));

    return block;
}



/*
-----------------------------------------------------------------------------

    CActiveQueue

    FreeBlockL()

    Deallocates a block allocated with AllocateBlockL()

-----------------------------------------------------------------------------
*/

void CActiveQueue::FreeBlockL(TActiveQueueBlock *aBlock)
{
    __ASSERT_DEBUG((((aBlock->iPrevBlock != aBlock->iNextBlock) ||
                     (aBlock->iNextBlock == 0)) &&
                    ((aBlock == iBlocks) || (aBlock->iPrevBlock != 0))),
                   User::Panic(_L("CActiveQueue"), EInternalAssertionFailure));
    
    // Remove the block from the list of all blocks:
    if ( aBlock->iPrevBlock )
        aBlock->iPrevBlock->iNextBlock = aBlock->iNextBlock;
    else
        iBlocks = aBlock->iNextBlock;
    if ( aBlock->iNextBlock )
        aBlock->iNextBlock->iPrevBlock = aBlock->iPrevBlock;

    // Free the data area
    TUint8 *data = (TUint8*) aBlock->Ptr();    
    User::Free(data);

    // Free the block:
    delete aBlock;

    __ASSERT_DEBUG(((!iBlocks) ||
                    (((iBlocks->iNextBlock == 0) ||
                      (iBlocks->iNextBlock != iBlocks)) &&
                     (iBlocks->iPrevBlock == 0))),
                   User::Panic(_L("CActiveQueue"), EInternalAssertionFailure));
}





// End of File
