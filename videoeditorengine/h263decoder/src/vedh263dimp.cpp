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
* H.263 Decoder API implementation class.
*
*/



// INCLUDE FILES
#include "vde.h"
#include "h263dapi.h"
#include "vedh263dimp.h"
/* MVE */
#include "MPEG4Transcoder.h"

// An assertion macro wrapper to clean up the code a bit
#define DLASSERT(x) __ASSERT_DEBUG(x, User::Panic(_L("CVedH263Dec"), EInternalAssertionFailure))


// ============================ MEMBER FUNCTIONS ===============================

// -----------------------------------------------------------------------------
// CVedH263DecImp::CVedH263DecImp
// C++ default constructor can NOT contain any code, that
// might leave.
// -----------------------------------------------------------------------------
//
CVedH263DecImp::CVedH263DecImp(TSize aFrameSize, TInt /*aNumReferencePictures*/)
{
    iH263dHandle = 0;
    iFrameSize = aFrameSize;
}

// -----------------------------------------------------------------------------
// CVedH263DecImp::ConstructL
// Symbian 2nd phase constructor can leave.
// -----------------------------------------------------------------------------
//
void CVedH263DecImp::ConstructL()
{

    // Open a decoder instance
    h263dOpen_t openParams;
    openParams.numPreallocatedFrames = 1;
    openParams.lumWidth = iFrameSize.iWidth;
    openParams.lumHeight = iFrameSize.iHeight;

    openParams.fRPS = 0; // reference picture selection mode not in use
    openParams.numReferenceFrames = 1;
    openParams.freeSpace = 0;
    iH263dHandle = h263dOpen(&openParams);
    if (!iH263dHandle)
        User::Leave(KErrGeneral);

}

// -----------------------------------------------------------------------------
// CVedH263DecImp::NewL
// Two-phased constructor.
// -----------------------------------------------------------------------------
//
EXPORT_C CVedH263Dec* CVedH263Dec::NewL(const TSize aFrameSize, const TInt aNumReferencePictures)
{

    CVedH263DecImp* self = new (ELeave) CVedH263DecImp(aFrameSize, aNumReferencePictures);
    CleanupStack::PushL(self);
    self->ConstructL();
    CleanupStack::Pop(self);    
    return self;    
}


// -----------------------------------------------------------------------------
// CVedH263DecImp::~CVedH263DecImp
// Destructor
// -----------------------------------------------------------------------------
//
CVedH263DecImp::~CVedH263DecImp()
{
    // Close the decoder instance if one has been opened
    if (iH263dHandle)
    {
        h263dClose(iH263dHandle);
        iH263dHandle = 0;
    }

}


// -----------------------------------------------------------------------------
// CVedH263DecImp::SetRendererL
// Sets the renderer object which the decoder will use
// -----------------------------------------------------------------------------
//
void CVedH263DecImp::SetRendererL(MVideoRenderer* /*aRenderer*/)
{
    // this method is not supported any more
    User::Panic(_L("CVedH263Dec - method not supported"), EInternalAssertionFailure);
}

// -----------------------------------------------------------------------------
// CVedH263DecImp::SetPostFilterL
// Sets the post-filter to be used in decoding
// -----------------------------------------------------------------------------
//
void CVedH263DecImp::SetPostFilterL(const TPostFilter /*aFilter*/)
{
    // this method is not supported any more
    User::Panic(_L("CVedH263Dec - method not supported"), EInternalAssertionFailure);
}

// -----------------------------------------------------------------------------
// CVedH263DecImp::GetYUVFrame
// Retrieves the YUV buffer for the last decoded frame
// -----------------------------------------------------------------------------
//
TUint8* CVedH263DecImp::GetYUVFrame()
{
    // this method is not supported any more
    User::Panic(_L("CVedH263Dec - method not supported"), EInternalAssertionFailure);

    return NULL; 

}

// -----------------------------------------------------------------------------
// CVedH263DecImp::GetYUVBuffers
// Retrieves the Y/U/V buffers for the given frame
// -----------------------------------------------------------------------------
//
TInt CVedH263DecImp::GetYUVBuffers(const TAny */*aFrame*/, TUint8*& /*aYFrame*/, TUint8*& /*aUFrame*/, 
                                   TUint8*& /*aVFrame*/, TSize& /*aFrameSize*/)
{
    // this method is not supported any more
    User::Panic(_L("CVedH263Dec - method not supported"), EInternalAssertionFailure);
    return KErrNotSupported;
}

// -----------------------------------------------------------------------------
// CVedH263DecImp::DecodeFrameL
// Decodes / Transcodes a compressed frame
// -----------------------------------------------------------------------------
//

void CVedH263DecImp::DecodeFrameL(const TPtrC8& aInputBuffer, TPtr8& aOutputBuffer,
                                  TBool& aFirstFrame, TInt& aBytesDecoded,
                                  vdeDecodeParamters_t *aDecoderInfo)
{
        
    TInt error = KErrNone;
    
    TInt bytesProduced = 0;

    error = h263dDecodeFrameL(iH263dHandle, (TAny*)aInputBuffer.Ptr(), (TAny*)aOutputBuffer.Ptr(), 
                             aInputBuffer.Length(), (TUint8*)&aFirstFrame, &aBytesDecoded, 
                             &bytesProduced, aDecoderInfo);    

    aOutputBuffer.SetLength(bytesProduced);
    
    switch ( error )
        {
        case H263D_ERROR_NO_INTRA:
            {
            User::Leave( EDecoderNoIntra );
            }
            break;
        case H263D_OK_BUT_FRAME_USELESS:
        case H263D_OK_BUT_BIT_ERROR:
            {
            User::Leave( EDecoderCorrupted );
            }
            break;
        case H263D_OK:
        case H263D_OK_EOS:
        case H263D_OK_BUT_NOT_CODED:
            {
            // ok
            return;
            }
        default:
            {
            User::Leave(EDecoderFailure);
            }
            break;

        }
    
}


// -----------------------------------------------------------------------------
// CVedH263DecImp::DecodeFrameL
// Decodes / Transcodes a compressed frame
// -----------------------------------------------------------------------------
//
void CVedH263DecImp::DecodeFrameL(const TPtrC8& /*aInputBuffer*/, TPtr8& /*aOutputBuffer*/,
                                  TBool& /*aFirstFrame*/, TInt& /*aBytesDecoded*/, 
                                  const TColorEffect /*aColorEffect*/,
                                  const TBool /*aGetDecodedFrame*/, TInt /*aFrameOperation*/,
                                  TInt* /*aTrP*/, TInt* /*aTrD*/, TInt /*aVideoClipNumber*/, TInt /*aSMSpeed*/, 
                                  TInt /*aDataFormat*/)
{
    // this method is not supported any more
    User::Panic(_L("CVedH263Dec - method not supported"), EInternalAssertionFailure);
}

// -----------------------------------------------------------------------------
// CVedH263DecImp::DecodeFrameL
// Decodes a compressed frame
// -----------------------------------------------------------------------------
//      
void CVedH263DecImp::DecodeFrameL(const TPtrC8& /*aInputBuffer*/, TBool& /*aFirstFrame*/, TInt& /*aBytesDecoded*/, 
                                                                    TInt /*aDataFormat*/)
{    
    // this method is not supported any more
    User::Panic(_L("CVedH263Dec - method not supported"), EInternalAssertionFailure);
}

// -----------------------------------------------------------------------------
// CVedH263DecImp::FrameValid
// Checks if the given frame is valid
// -----------------------------------------------------------------------------
//
TBool CVedH263DecImp::FrameValid(const TAny */*aFrame*/)
{
    return ETrue;

}

// -----------------------------------------------------------------------------
// CVedH263DecImp::FrameRendered
// Returns the given frame to the decoder after it is no longer needed
// -----------------------------------------------------------------------------
//
void CVedH263DecImp::FrameRendered(const TAny */*aFrame*/)
{
    // Return frame to the decoder rendering subsystem
}

// -----------------------------------------------------------------------------
// CVedH263DecImp::CheckVOSHeaderL
// Check and change the VOS header (resync marker bit needs to be set) 
// -----------------------------------------------------------------------------
//
TBool CVedH263DecImp::CheckVOSHeaderL(TPtrC8& aInputBuffer)
{
   return vdtChangeVosHeaderRegResyncL(aInputBuffer, aInputBuffer.Length());

}





// End of file



