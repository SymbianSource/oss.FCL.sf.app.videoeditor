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
* Header for VOL header parser.
*
*/



#ifndef __VEDVOLREADER_H__
#define __VEDVOLREADER_H__


//  INCLUDES
#include <e32base.h>
#include <e32std.h>
#include <vedcommon.h>





// DATA TYPES

/**
 * Structure for the Video Object Layer header data
 */ 
struct TVolHeader
    {
    TInt iProfileLevelId;     // Indicates the Level Id (0,1,2,3,4,8 or 9) of the Simple
                              // Profile the Video Object conforms to
    TInt iVoPriority;         // Priority assigned to the Video Object 
                              // a value between 1(lowest)-7(highest).
                              // If not in the bitstream, set to zero
   
    TInt iVoId;               // id of the Video Object 
    TInt iVolId;              // id of the Video Object Layer 
    TInt iRandomAccessibleVol;// set to 1 if all the VOPs in the stream are I-VOP.
    TInt iPixelAspectRatio;   // see MPEG-4 visual spec. pp. 71

    TInt iTimeIncrementResolution;
                              // Resolution to interpret the time_increment 
                              // fields in the VOP headers 

    TInt iLumWidth;           // Frame width of the Y component in pixels 
    TInt iLumHeight;          // Frame height of the Y component in pixels 

    TUint8 iErrorResDisable;  // Flag ON if no resynchronization markers are
                              // used inside frames. 
                              // When OFF it doesn't mean they ARE used.
    TUint8 iDataPartitioned;  // Flag indicating if data partitioning inside 
                              // a VP is used or not.
    TUint8 iReversibleVlc;    // flag indicating the usage of reversible 
                              // VLC codes

    // the following parameters concern the input video signal,
    // see MPEG-4 visual spec. pp. 66-70, and "Note" in viddemux_mpeg.c 
    // Not used in the current implementatnion.
    TInt iVideoFormat;
    TInt iVideoRange;
    TInt iColourPrimaries;    
    TInt iTransferCharacteristics;
    TInt iMatrixCoefficients;

    // the following parameters are used in the Video Buffering Verifier
    // (Annex D) to monitor the input bit buffer, complexity, memory buffer
    // used in the decoding process. The conformance of a stream can be checked
    // using these parameters. 
    // Not used in the current implementatnion. 
    TUint32 iBitRate;
    TUint32 iVbvBufferSize;
    TUint32 iVbvOccupancy;

    HBufC8* iUserData;        // User Data if available
    };
    


// CLASS DECLARATION

/**
 * A class for parsing the Video Object Layer header
 */
class CVedVolReader : public CBase
    {
public:

    /**
     * Two-phased constructor.
     */
    IMPORT_C static CVedVolReader* NewL();

    /**
     * Destructor.
     */
    virtual ~CVedVolReader();
    
    /**
     * Parses given Video Object Layer header
     * @param aData Buffer from where to parse the header
     * @return error code
     */
    IMPORT_C TInt ParseVolHeaderL(TDesC8& aData);
    
    /**
     * Returns the time increment resolution that was read from the VOL header
     * @return time increment resolution
     */    
    IMPORT_C TInt TimeIncrementResolution() const;
    
    /**
     * Returns the width of the video that was read from the VOL header
     * @return width
     */  
    IMPORT_C TInt Width() const;
    
    /**
     * Returns the height of the video that was read from the VOL header
     * @return height
     */  
    IMPORT_C TInt Height() const;
    
    /**
     * Returns the Level Id of the Simple Profile the Video Object conforms to
     * @return profile level Id
     */  
    IMPORT_C TInt ProfileLevelId() const;
    
    /**
     * Returns the bitstream mode of the video
     * @return bitstream mode
     */  
    IMPORT_C TVedVideoBitstreamMode BitstreamMode() const;
    
    /**
     * Returns the size of the VOL header
     * @return size of the header
     */  
    IMPORT_C TInt HeaderSize() const;

private:

    /**
     * Structure for internal buffer of the header sequence
     */
    struct TSeqHeaderBuffer
        {
        TDesC8& iData;           // The actual data of the buffer
        TInt    iGetIndex;       // Index of getting from the buffer 
        TInt    iBitInOctet;	 // Bit index in the buffer
        
        TSeqHeaderBuffer(TDesC8& aData, TInt aGetIndex, TInt aBitInOctet) :
            iData(aData), iGetIndex(aGetIndex), iBitInOctet(aBitInOctet) {}
        };
    
    /**
     * C++ default constructor.
     */
    CVedVolReader();
    
    /**
     * By default Symbian OS constructor is private.
     */
    void ConstructL();
    
    /**
     * Reads requested bits from given buffer
     * @param aBuffer Buffer from where to read the bits
     * @param aNumBits Amount of bits to read
     * @param aFlush Discard the bits that were read
     * @return The bits that were read
     */
    TUint32 ReadSeqHeaderBits(TSeqHeaderBuffer& aBuffer, TInt aNumBits, TBool aFlush);
    
    /**
     * Reads user data from given buffer
     * @param aBuffer Buffer from where to read the user data
     */
    void ReadUserDataL(TSeqHeaderBuffer& aBuffer);
    
    /**
     * Checks what is the bit stream mode the video
     * @param aErd Flag error resilience disable used
     * @param aDp Flag data partitioned used
     * @param aRvlc Flag reversible vlc used
     * @return The bit stream mode
     */
    TVedVideoBitstreamMode CheckBitstreamMode(TUint8 aErd, TUint8 aDp, TUint8 aRvlc);

private:

    // Member variables
    
    static const TInt KMaxUserDataLength;       // The maximum allocated memory for 
                                                // user data (UD). Only this much of the UD
                                                // from the bitstream is stored
                                                
    static const TUint8 KMsbMask[8];            // Mask for extracting the needed bits
    static const TUint8 KLsbMask[9];
    
    TVolHeader iHeader;                         // For storing the header data
    
    TVedVideoBitstreamMode iBitstreamMode;
    TInt iHeaderSize;

    };

#endif // __VEDVOLREADER_H__

