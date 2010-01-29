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
* MPEG-4 VOL header parsing.
*
*/


// INCLUDE FILES
#include "mpegcons.h"
#include "VedVolReader.h"



// MACROS
#ifdef _DEBUG
#include <e32svr.h>
#define PRINT(x) RDebug::Print x;
#else
#define PRINT(x)
#endif



// CONSTANTS
const TUint8 CVedVolReader::KMsbMask[] = {1, 3, 7, 15, 31, 63, 127, 255};
const TUint8 CVedVolReader::KLsbMask[] = {255, 254, 252, 248, 240, 224, 192, 128, 0};

const TInt CVedVolReader::KMaxUserDataLength = 512;



// ================= MEMBER FUNCTIONS =======================

// Two-phased constructor
EXPORT_C CVedVolReader* CVedVolReader::NewL()
    {
    CVedVolReader* self = new (ELeave) CVedVolReader();
    CleanupStack::PushL(self);
    self->ConstructL();
    CleanupStack::Pop(self);
    return self;
    }


// C++ default constructor
CVedVolReader::CVedVolReader() : iBitstreamMode(EVedVideoBitstreamModeUnknown)
    {
    // Reset the header data to zeroes
    Mem::FillZ(&iHeader, sizeof(TVolHeader));
    }

// Symbian OS default constructor can leave
void CVedVolReader::ConstructL()
    {
    }

// Destructor
CVedVolReader::~CVedVolReader()
    {
    if (iHeader.iUserData != 0)
        {
        delete iHeader.iUserData;
        }
    }
    
// ---------------------------------------------------------
// CVedVolReader::ParseVolHeaderL
// Parses given Video Object Layer header
// ---------------------------------------------------------
//
EXPORT_C TInt CVedVolReader::ParseVolHeaderL(TDesC8& aData)
    {
    TInt useDefaultVBVParams = 0;
    TInt numBits;
    TUint32 bits;
    
    // Reset the header data
    if (iHeader.iUserData != 0)
        {
        delete iHeader.iUserData;
        iHeader.iUserData = 0;
        }
    Mem::FillZ(&iHeader, sizeof(TVolHeader));
    iBitstreamMode = EVedVideoBitstreamModeUnknown;
    iHeaderSize = 0;
    
    // Check that we have something to parse
    if (aData.Length() == 0)
        {
        User::Leave(KErrNotFound);
        }
    
    TSeqHeaderBuffer buffer(aData, 0, 7);   // Bit 7 is the first bit in a byte   
    
    // if H.263 PSC is present
    bits = ReadSeqHeaderBits(buffer, 22, EFalse);
    if (bits == 32)
        {
        // This is H.263 so there are no VOL headers
        iBitstreamMode = EVedVideoBitstreamModeH263;
        return KErrNone;
        }

    // if Visual Object Sequence Start Code is present 
    bits = ReadSeqHeaderBits(buffer, MP4_VOS_START_CODE_LENGTH, EFalse);
    if (bits == MP4_VOS_START_CODE)
        {
        // vos_start_code
        ReadSeqHeaderBits(buffer, MP4_VOS_START_CODE_LENGTH, ETrue);

        // profile_and_level_indication (8 bits) 
        bits = ReadSeqHeaderBits(buffer, 8, ETrue);      
        //   8: Simple Profile Level 0 (from ISO/IEC 14496-2:1999/FPDAM4 [N3670] October 2000)
        //   9: Simple Profile Level 0b
        //   1: Simple Profile Level 1
        //   2: Simple Profile Level 2
        //   3: Simple Profile Level 3
        //   4: Simple Profile Level 4a
        if (bits != 1 && bits != 2 && bits != 3 && bits != 4 && bits != 8 && bits != 9)
            {
            // Profile level id was not recognized so take a guess
            // This is changed later if resolution does not match
            bits = 9;
            }
            
        iHeader.iProfileLevelId = (TInt) bits;
            
        ReadUserDataL(buffer);
        }

    // if Visual Object Start Code is present 
    bits = ReadSeqHeaderBits(buffer, MP4_VO_START_CODE_LENGTH, EFalse);
    if (bits == MP4_VO_START_CODE)
        {
        // visual_object_start_code
        ReadSeqHeaderBits(buffer, MP4_VO_START_CODE_LENGTH, ETrue);

        // is_visual_object_identifier (1 bit)
        bits = ReadSeqHeaderBits(buffer, 1, ETrue);
        if (bits)
            {
            // visual_object_ver_id (4 bits)
            bits = ReadSeqHeaderBits(buffer, 4, ETrue);
            if (bits != 1 && bits != 2)
                {
                // this is not an MPEG-4 version 1 or 2 stream
                PRINT((_L("CVedVolReader: ERROR - This is not an MPEG-4 version 1 or 2 stream")))
                User::Leave(KErrNotSupported);
                }

            // visual_object_priority (3 bits) 
            bits = ReadSeqHeaderBits(buffer, 3, ETrue);
            iHeader.iVoPriority = (TInt) bits;
            } 
        else
            {
            iHeader.iVoPriority = 0;
            }

        // visual_object_type (4 bits)
        bits = ReadSeqHeaderBits(buffer, 4, ETrue);
        if (bits != 1)
            {
            // this is not a video object
            PRINT((_L("CVedVolReader: ERROR - This is not a video object")))
            User::Leave(KErrNotSupported);
            }

        // is_video_signal_type (1 bit)
        bits = ReadSeqHeaderBits(buffer, 1, ETrue);
        if (bits)
            {
            /* Note: The following fields in the bitstream give information about the 
               video signal type before encoding. These parameters don't influence the 
               decoding algorithm, but the composition at the output of the video decoder.
               Until a way to utilize them is found, these fields are just dummyly read,
               but not interpreted.
               For interpretation see the MPEG-4 Visual standard page 66-70. */

            // video_format (3 bits)
            bits = ReadSeqHeaderBits(buffer, 3, ETrue);
            iHeader.iVideoFormat = (TInt) bits;

            // video_range (1 bit)
            bits = ReadSeqHeaderBits(buffer, 1, ETrue);
            iHeader.iVideoRange = (TInt) bits;

            // colour_description (1 bit)
            bits = ReadSeqHeaderBits(buffer, 1, ETrue);
            if (bits)
                {          
                // colour_primaries (8 bits)
                bits = ReadSeqHeaderBits(buffer, 8, ETrue);
                iHeader.iColourPrimaries = (TInt) bits;

                // transfer_characteristics (8 bits)
                bits = ReadSeqHeaderBits(buffer, 8, ETrue);
                iHeader.iTransferCharacteristics = (TInt) bits;

                // matrix_coefficients (8 bits)
                bits = ReadSeqHeaderBits(buffer, 8, ETrue);
                iHeader.iMatrixCoefficients = (TInt) bits;
                }
            else
                {
                iHeader.iColourPrimaries = 1;         // default: ITU-R BT.709 
                iHeader.iTransferCharacteristics = 1; // default: ITU-R BT.709 
                iHeader.iMatrixCoefficients = 1;      // default: ITU-R BT.709 
                }
            } 
        else
            {
            // default values
            iHeader.iVideoFormat = 5;             // Unspecified video format 
            iHeader.iVideoRange = 0;              // Y range 16-235 pixel values 
            iHeader.iColourPrimaries = 1;         // ITU-R BT.709 
            iHeader.iTransferCharacteristics = 1; // ITU-R BT.709 
            iHeader.iMatrixCoefficients = 1;      // ITU-R BT.709 
            }

        // check the next start code
        ReadSeqHeaderBits(buffer, 1, ETrue);
        if (buffer.iBitInOctet != 7)
            {
            numBits = buffer.iBitInOctet + 1;

            bits = ReadSeqHeaderBits(buffer, numBits, ETrue);
            if (bits != (TUint32) ((1 << numBits)-1))
                {
                // stuffing error in VO
                PRINT((_L("CVedVolReader: ERROR - Stuffing error")))
                User::Leave(KErrNotSupported);
                }
            }

        ReadUserDataL(buffer);
        }
   
    // if Video Object Start Code is present
    bits = ReadSeqHeaderBits(buffer, MP4_VID_START_CODE_LENGTH, EFalse);
    if (bits == MP4_VID_START_CODE)
        {
        // video_object_start_code 
        ReadSeqHeaderBits(buffer, MP4_VID_START_CODE_LENGTH, ETrue);

        // video_object_id
        bits = ReadSeqHeaderBits(buffer, MP4_VID_ID_CODE_LENGTH, ETrue);
        iHeader.iVoId = (TInt) bits;
        }
        
    // Check if H.263 PSC follows the VO header, in which case this is MPEG-4 with short header
    bits = ReadSeqHeaderBits(buffer, 22, EFalse);
    if (bits == 32 || buffer.iData.Length() <= buffer.iGetIndex)
        {
        iBitstreamMode = EVedVideoBitstreamModeMPEG4ShortHeader;
        
        // Calculate the header size in bytes
        if (buffer.iBitInOctet == 7)
            {
            // The size is a multiple of 8 bits so the size is the number of bytes we've read so far
            iHeaderSize = buffer.iGetIndex;
            }
        else
            {
            // Round up to the next full byte
            iHeaderSize = buffer.iGetIndex + 1;
            }
        
        // We no longer support short header clips    
        User::Leave(KErrNotSupported);
        }

    // vol_start_code 
    bits = ReadSeqHeaderBits(buffer, MP4_VOL_START_CODE_LENGTH, ETrue);
    if(bits != MP4_VOL_START_CODE)
    {
        PRINT((_L("CVedVolReader: ERROR - Bitstream does not start with MP4_VOL_START_CODE")))
        User::Leave(KErrCorrupt);
    }

    // vol_id
    bits = ReadSeqHeaderBits(buffer, MP4_VOL_ID_CODE_LENGTH, ETrue);
    iHeader.iVolId = (TInt) bits;

    // random_accessible_vol (1 bit)
    bits = ReadSeqHeaderBits(buffer, 1, ETrue);
    iHeader.iRandomAccessibleVol = (TUint8) bits;

    // video_object_type_indication (8 bits)
    bits = ReadSeqHeaderBits(buffer, 8, ETrue);
    if (bits != 1)
        {
        // this is not a simple video object stream 
        PRINT((_L("CVedVolReader: ERROR - This is not a simple video object stream")))
        User::Leave(KErrNotSupported);
        }

    // is_object_layer_identifier (1 bit)
    bits = ReadSeqHeaderBits(buffer, 1, ETrue);
    if (bits)
        {
        // visual_object_ver_id (4 bits)
        bits = ReadSeqHeaderBits(buffer, 4, ETrue);
        if (bits != 1 && bits != 2)
            {
            // this is not an MPEG-4 version 1 or 2 stream
            PRINT((_L("CVedVolReader: ERROR - This is not an MPEG-4 version 1 or 2 stream")))
            User::Leave(KErrNotSupported);
            }

        // video_object_layer_priority (3 bits)
        bits = ReadSeqHeaderBits(buffer, 3, ETrue);
        iHeader.iVoPriority = (TInt) bits;
        } 

    // aspect_ratio_info: `0010`- 12:11 (625-type for 4:3 picture)
    bits = ReadSeqHeaderBits(buffer, 4, ETrue);
    iHeader.iPixelAspectRatio = (TInt) bits;

    // extended par 
    if (bits == 15)
        { 
        // par_width 
        bits = ReadSeqHeaderBits(buffer, 8, ETrue);
        // par_height 
        bits = ReadSeqHeaderBits(buffer, 8, ETrue);
        }
   
    // vol_control_parameters flag 
    bits = ReadSeqHeaderBits(buffer, 1, ETrue);
    if (bits)
        {
        // chroma_format (2 bits)
        bits = ReadSeqHeaderBits(buffer, 2, ETrue);
        if (bits != 1)
            {
            PRINT((_L("CVedVolReader: ERROR - Not supported chroma format")))
            User::Leave(KErrNotSupported);
            }

        // low_delay (1 bits)
        bits = ReadSeqHeaderBits(buffer, 1, ETrue);

        // vbv_parameters (1 bits)
        bits = ReadSeqHeaderBits(buffer, 1, ETrue);
        if (bits)
            {
            // first_half_bitrate (15 bits) 
            bits = ReadSeqHeaderBits(buffer, 15, ETrue);
            iHeader.iBitRate = (bits << 15);
          
            // marker_bit
            if (!ReadSeqHeaderBits(buffer, 1, ETrue))
                {
                User::Leave(KErrCorrupt);
                }
           
            // latter_half_bitrate (15 bits)
            bits = ReadSeqHeaderBits(buffer, 15, ETrue);
            iHeader.iBitRate += bits;
            iHeader.iBitRate *= 400;
           
            // marker_bit
            if (!ReadSeqHeaderBits(buffer, 1, ETrue))
                {
                User::Leave(KErrCorrupt);
                }
           
            // first_half_vbv_buffer_size (15 bits)
            bits = ReadSeqHeaderBits(buffer, 15, ETrue);
            iHeader.iVbvBufferSize = (bits << 3);
           
            // marker_bit
            if (!ReadSeqHeaderBits(buffer, 1, ETrue))
                {
                User::Leave(KErrCorrupt);
                }
           
            // latter_half_vbv_buffer_size (3 bits)
            bits = ReadSeqHeaderBits(buffer, 3, ETrue);
            iHeader.iVbvBufferSize += bits;
            iHeader.iVbvBufferSize *= 16384;
           
            // first_half_vbv_occupancy (11 bits)
            bits = ReadSeqHeaderBits(buffer, 11, ETrue);
            iHeader.iVbvOccupancy = (bits << 15);
           
            // marker_bit
            if (!ReadSeqHeaderBits(buffer, 1, ETrue))
                {
                User::Leave(KErrCorrupt);
                }
           
            // latter_half_vbv_occupancy (15 bits) 
            bits = ReadSeqHeaderBits(buffer, 15, ETrue);
            iHeader.iVbvOccupancy += bits;
            iHeader.iVbvOccupancy *= 64;
           
            // marker_bit
            if (!ReadSeqHeaderBits(buffer, 1, ETrue))
                {
                User::Leave(KErrCorrupt);
                }
            }
        else
            {
            useDefaultVBVParams = 1;
            }
        }     
    else
        {
        useDefaultVBVParams = 1;
        }

    // vol_shape (2 bits) 
    bits = ReadSeqHeaderBits(buffer, 2, ETrue);
    // rectangular_shape = '00'
    if (bits != 0)
        {
        PRINT((_L("CVedVolReader: ERROR - Not rectangular shape is not supported")))
        User::Leave(KErrNotSupported);
        }

    // marker_bit
    if (!ReadSeqHeaderBits(buffer, 1, ETrue))
        {
        User::Leave(KErrCorrupt);
        }
    
    // time_increment_resolution 
    bits = ReadSeqHeaderBits(buffer, 16, ETrue);
    iHeader.iTimeIncrementResolution = (TInt) bits;
 
    // marker_bit
    if (!ReadSeqHeaderBits(buffer, 1, ETrue))
        {
        User::Leave(KErrCorrupt);
        }
    
    // fixed_vop_rate 
    bits = ReadSeqHeaderBits(buffer, 1, ETrue);

    // fixed_vop_time_increment (1-15 bits)
    if (bits)
        {
        for (numBits = 1; ((iHeader.iTimeIncrementResolution-1) >> numBits) != 0; numBits++) 
            {
            }

        bits = ReadSeqHeaderBits(buffer, numBits, ETrue);
        }

    // marker_bit
    if (!ReadSeqHeaderBits(buffer, 1, ETrue))
        {
        User::Leave(KErrCorrupt);
        }
    
    // vol_width (13 bits) 
    bits = ReadSeqHeaderBits(buffer, 13, ETrue);
    iHeader.iLumWidth = (TInt) bits;

    // marker_bit
    if (!ReadSeqHeaderBits(buffer, 1, ETrue))
        {
        User::Leave(KErrCorrupt);
        }
    
    // vol_height (13 bits) 
    bits = ReadSeqHeaderBits(buffer, 13, ETrue);
    iHeader.iLumHeight = (TInt) bits;
    
    // Accept only resolutions that are divisible by 16
    if ( ((iHeader.iLumWidth & 0x0000000f) != 0) ||
        ((iHeader.iLumHeight & 0x0000000f) != 0) )
        {
        PRINT((_L("CVedVolReader: ERROR - Unsupported resolution")))
        User::Leave(KErrNotSupported);
        }
        
    TInt macroBlocks = iHeader.iLumWidth * iHeader.iLumHeight / (16 * 16);
    
    // Check that we don't have too many macro blocks (ie resolution is not too big)
    if( macroBlocks > 1200 )
        {
        PRINT((_L("CVedVolReader: ERROR - Unsupported resolution")))
        User::Leave(KErrNotSupported);
        }
        
    // Check that profile level id matches with the number of macro blocks
    if( macroBlocks > 396 )
        {
        // Resolution is higher than CIF => level must be 4a
        if( iHeader.iProfileLevelId != 4 )
            {
            iHeader.iProfileLevelId = 4;
            useDefaultVBVParams = 1;
            }
        }
    else if( macroBlocks > 99 )
        {
        // Resolution is higher than QCIF => level must be atleast 2
        if( iHeader.iProfileLevelId < 2 || iHeader.iProfileLevelId > 4 )
            {
            iHeader.iProfileLevelId = 3;    // QVGA/CIF @ 30fps
            useDefaultVBVParams = 1;
            }
        }
    
    // Set default VBV params if not set already  
    if (useDefaultVBVParams)
        {
        switch (iHeader.iProfileLevelId)
            {
            case 1:
                iHeader.iVbvBufferSize = 10;
                iHeader.iBitRate = 64;
                break;
            case 2:
                iHeader.iVbvBufferSize = 20;
                iHeader.iBitRate = 128;
                break;
            case 4:
                iHeader.iVbvBufferSize = 80;
                iHeader.iBitRate = 4000;
                break;
            case 8:
                iHeader.iVbvBufferSize = 10;
                iHeader.iBitRate = 64;
                break;
            case 9:
                iHeader.iVbvBufferSize = 20;
                iHeader.iBitRate = 128;
                break;
            default:
                iHeader.iVbvBufferSize = 20;
                iHeader.iBitRate = 384;
                break;
            }
        
        iHeader.iVbvOccupancy = iHeader.iVbvBufferSize * 170;
       
        iHeader.iVbvOccupancy *= 64;
        iHeader.iVbvBufferSize *= 16384;
        iHeader.iBitRate *= 1024;
        }
    
    // marker_bit
    if (!ReadSeqHeaderBits(buffer, 1, ETrue))
        {
        User::Leave(KErrCorrupt);
        }
    
    // interlaced (1 bit)
    bits = ReadSeqHeaderBits(buffer, 1, ETrue);
    if (bits)
        {
        PRINT((_L("CVedVolReader: ERROR - Interlaced VOP not supported")))
        User::Leave(KErrNotSupported);
        }

    // OBMC_disable
    bits = ReadSeqHeaderBits(buffer, 1, ETrue);
    if (!bits)
        {
        PRINT((_L("CVedVolReader: ERROR - Overlapped motion compensation not supported")))
        User::Leave(KErrNotSupported);
        }

    // sprite_enable (1 bit) 
    bits = ReadSeqHeaderBits(buffer, 1, ETrue);
    if (bits)
        {
        PRINT((_L("CVedVolReader: ERROR - Sprites not supported")))
        User::Leave(KErrNotSupported);
        }

    // not_8_bit (1 bit) 
    bits = ReadSeqHeaderBits(buffer, 1, ETrue);
    if (bits)
        {
        PRINT((_L("CVedVolReader: ERROR - Not 8 bits/pixel not supported")))
        User::Leave(KErrNotSupported);
        }

    // quant_type (1 bit) 
    bits = ReadSeqHeaderBits(buffer, 1, ETrue);
    if (bits)
        {
        PRINT((_L("CVedVolReader: ERROR - H.263/MPEG-2 Quant Table switch not supported")))
        User::Leave(KErrNotSupported);
        }

    // complexity_estimation_disable (1 bit) 
    bits = ReadSeqHeaderBits(buffer, 1, ETrue);
    if (!bits)
        {
        PRINT((_L("CVedVolReader: ERROR - Complexity estimation header not supported")))
        User::Leave(KErrNotSupported);
        }
  
    // resync_marker_disable (1 bit)
    bits = ReadSeqHeaderBits(buffer, 1, ETrue);
    iHeader.iErrorResDisable = (TUint8) bits;

    // data_partitioned (1 bit) 
    bits = ReadSeqHeaderBits(buffer, 1, ETrue);
    iHeader.iDataPartitioned = (TUint8) bits;

    if (iHeader.iDataPartitioned)
        {
        // reversible_vlc (1 bit)
        bits = ReadSeqHeaderBits(buffer, 1, ETrue);
        iHeader.iReversibleVlc = (TUint8) bits;
        }

    // scalability (1 bit)
    bits = ReadSeqHeaderBits(buffer, 1, ETrue);
    if (bits)
        {
        PRINT((_L("CVedVolReader: ERROR - Scalability not supported")))
        User::Leave(KErrNotSupported);
        }

    // check the next start code
    ReadSeqHeaderBits(buffer, 1, ETrue);
    if (buffer.iBitInOctet != 7)
        {
        numBits = buffer.iBitInOctet + 1;
       
        bits = ReadSeqHeaderBits(buffer, numBits, ETrue);
        
        /* Removed temporarily
        if (bits != (TUint32) ((1 << numBits)-1))
            {
            // this is not a video object
            PRINT((_L("CVedVolReader: ERROR - Stuffing error")))
            User::Leave(KErrNotSupported);
            }*/
        }

    ReadUserDataL(buffer);
        
    // Calculate the header size in bytes
    if (buffer.iBitInOctet == 7)
        {
        // The size is a multiple of 8 bits so the size is the number of bytes we've read so far
        iHeaderSize = buffer.iGetIndex;
        }
    else
        {
        // Round up to the next full byte
        iHeaderSize = buffer.iGetIndex + 1;
        }
     
    // Determine the bitstream mode  
    iBitstreamMode = CheckBitstreamMode(iHeader.iErrorResDisable, iHeader.iDataPartitioned, iHeader.iReversibleVlc);

    // If no error in bit buffer functions
    return KErrNone;
    }

// ---------------------------------------------------------
// CVedVolReader::ReadSeqHeaderBits
// Reads requested bits from given buffer
// ---------------------------------------------------------
//  
TUint32 CVedVolReader::ReadSeqHeaderBits(TSeqHeaderBuffer& aBuffer, TInt aNumBits, TBool aFlush)
    {      
    TUint startIndex;       // the index of the first byte to read 
    TUint startMask;        // the mask for the first byte 
    TUint endIndex;         // the index of the last byte to read 
    TUint endMask;          // the mask for the last byte 
    TUint endShift;         // the number of shifts after masking the last byte 
    TUint endNumberOfBits;  // the number of bits in the last byte 
    TUint newBitIndex;      // bitIndex after getting the bits 
    TUint newGetIndex;      // getIndex after getting the bits 
    
    TUint32 returnValue = 0;

    startIndex = aBuffer.iGetIndex;
    startMask = KMsbMask[aBuffer.iBitInOctet];

    // Check that there are enough bits left
    if (startIndex * 8 + aNumBits > aBuffer.iData.Length() * 8)
        {
	    return 0;
        }

    if (aBuffer.iBitInOctet + 1 >= aNumBits)
        {
        // The bits are within one byte. 
        endShift = aBuffer.iBitInOctet - aNumBits + 1;
        endMask = KLsbMask[endShift];
        returnValue = (aBuffer.iData[startIndex] & startMask & endMask) >> endShift;
        if (endShift > 0)
            {
            newBitIndex = aBuffer.iBitInOctet - aNumBits;
            newGetIndex = aBuffer.iGetIndex;
            }
        else
            {
            newBitIndex = 7;
            newGetIndex = aBuffer.iGetIndex + 1;
            }
        } 
    else
        {
        // The bits are in multiple bytes. 
        aNumBits -= aBuffer.iBitInOctet + 1;
        endNumberOfBits = aNumBits & 7;

        newBitIndex = 7 - endNumberOfBits;
        newGetIndex = aBuffer.iGetIndex + (aNumBits >> 3) + 1;
      
        // Calculate the return value. 
        endIndex = newGetIndex;
        endShift = 8 - (aNumBits & 7);
        endMask = KLsbMask[endShift];
      
        returnValue = aBuffer.iData[startIndex] & startMask;
        startIndex++;
      
        while (startIndex != endIndex)
            {
            returnValue <<= 8;
            returnValue += (TUint8) aBuffer.iData[startIndex];
            startIndex++;
            }
        
        if (endNumberOfBits != 0)
            {
            returnValue <<= endNumberOfBits;
            returnValue += (aBuffer.iData[startIndex] & endMask) >> endShift;
            }
        }

    if (aFlush)
        {
        // Update indexes. 
        aBuffer.iGetIndex = newGetIndex;
        aBuffer.iBitInOctet = newBitIndex;
        }

    return returnValue;
    }

// ---------------------------------------------------------
// CVedVolReader::ReadUserDataL
// Reads user data from given buffer
// ---------------------------------------------------------
//    
void CVedVolReader::ReadUserDataL(TSeqHeaderBuffer& aBuffer)
    {
    TUint32 bits;
    
    // Check if User data is available 
    bits = ReadSeqHeaderBits(aBuffer, 32, EFalse);
    if (bits == MP4_USER_DATA_START_CODE)
        {
        if (iHeader.iUserData == 0)
            {
            iHeader.iUserData = HBufC8::NewL(KMaxUserDataLength);
            }
            
        TInt i = iHeader.iUserData->Length();    
        do 
            {
            bits = ReadSeqHeaderBits(aBuffer, 8, ETrue);
            if (i++ < KMaxUserDataLength)
                {
                (iHeader.iUserData->Des()).Append(TChar(bits));
                }
            }
        while (aBuffer.iData.Length() > aBuffer.iGetIndex &&
               ReadSeqHeaderBits(aBuffer, 24, EFalse) != 0x1);
        }
    }
     
// ---------------------------------------------------------
// CVedVolReader::CheckBitstreamMode
// Checks what is the bit stream mode the video
// ---------------------------------------------------------
//
TVedVideoBitstreamMode CVedVolReader::CheckBitstreamMode(TUint8 aErd, TUint8 aDp, TUint8 aRvlc)
    {
    TVedVideoBitstreamMode mode = EVedVideoBitstreamModeUnknown;
    int combination = ((!aErd) << 2) | (aDp << 1) | aRvlc;
                      
    switch (combination)
        {
        case 0:
            mode = EVedVideoBitstreamModeMPEG4Regular;
            break;
        case 2:
            mode = EVedVideoBitstreamModeMPEG4DP;
            break;
        case 3:
            mode = EVedVideoBitstreamModeMPEG4DP_RVLC;
            break;
        case 4:
            mode = EVedVideoBitstreamModeMPEG4Resyn;
            break;
        case 6:
            mode = EVedVideoBitstreamModeMPEG4Resyn_DP;
            break;
        case 7:
            mode = EVedVideoBitstreamModeMPEG4Resyn_DP_RVLC;
            break;
        default:
            break;
        }
        
    return mode;
    }
    
// ---------------------------------------------------------
// CVedVolReader::TimeIncrementResolution
// Returns the time increment resolution that was read from the VOL header
// ---------------------------------------------------------
//     
EXPORT_C TInt CVedVolReader::TimeIncrementResolution() const
    {
    return iHeader.iTimeIncrementResolution;
    }
       
// ---------------------------------------------------------
// CVedVolReader::Width
// Returns the width of the video that was read from the VOL header
// ---------------------------------------------------------
// 
EXPORT_C TInt CVedVolReader::Width() const
    {
    return iHeader.iLumWidth;
    }
      
// ---------------------------------------------------------
// CVedVolReader::Height
// Returns the height of the video that was read from the VOL header
// ---------------------------------------------------------
// 
EXPORT_C TInt CVedVolReader::Height() const
    {
    return iHeader.iLumHeight;
    }
    
// ---------------------------------------------------------
// CVedVolReader::ProfileLevelId
// Returns the Level Id of the Simple Profile the Video Object conforms to
// ---------------------------------------------------------
// 
EXPORT_C TInt CVedVolReader::ProfileLevelId() const
    {
    return iHeader.iProfileLevelId;
    }
    
// ---------------------------------------------------------
// CVedVolReader::BitstreamMode
// Returns the bitstream mode of the video
// ---------------------------------------------------------
//  
EXPORT_C TVedVideoBitstreamMode CVedVolReader::BitstreamMode() const
    {
    return iBitstreamMode;
    }
    
// ---------------------------------------------------------
// CVedVolReader::HeaderSize
// Returns the bitstream mode of the video
// ---------------------------------------------------------
//  
EXPORT_C TInt CVedVolReader::HeaderSize() const
    {
    return iHeaderSize;
    }

