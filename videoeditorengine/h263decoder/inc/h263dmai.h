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
* A header file defining the h.263 decoder API.               
*
*/


#ifndef H263DMAIN_H
#define H263DMAIN_H

#include <e32base.h>


/*
 * Structs and typedefs
 */
/* This structure is used to indicate the operating mode. */

/* Time change for merging MPEG-4 clips */
typedef struct MPEG4TimeParameter
{
    int modulo_time_base;       /* the time base of the video clip representing one second of duration */
    int time_inc;           /* the time increment from the synchronization point marked by modulo_time_base */
} tMPEG4TimeParameter;

/* Editing parameters passed to decoder for compressed domain editing */
typedef struct 
{
  TInt aColorEffect;	  /* special color effect (None/B&W/ColorTone) */
  TInt aColorToneU;       /* color tone value for U */
  TInt aColorToneV;       /* color tone value for V */
  TBool aGetVideoMode;    /* true if we need to get mode information from VOL */
  TBool fModeChanged;     /* true if bitstream mode needs to be changed (transcoding needed within MPEG-4) */ 
  TBool fHaveDifferentModes;  /* true if different MPEG-4 modes exist within movie */
  TBool aGetDecodedFrame; /* true if we need to return fully decoded frame */
  TInt aFrameOperation;   /* type of decoding operation neded: 1=EDecodeAndWrite,2=EDecodeNoWrite, 3=EWriteNoDecode */
  TInt aVideoClipNumber;  /* the index number of the video clip that this frame is part of */
  TInt aSMSpeed;          /* slow motion speed (25-100) */

  TInt *aTrP;             /* time increment value of previous frame */
  TInt *aTrD;             /* time duration of previous frame */
  tMPEG4TimeParameter * aMPEG4TimeStamp; /* time stamp of MPEG-4 frame */
  TInt aMPEG4DurationInMs;               /* time duration of MPEG-4 frame in millisec */
  TInt *aMPEG4TargetTimeResolution; /* time increment resolution of output video - returned from the decoder */
  TInt aFirstFrameQp;       /* QP of first frame, used for MPEG4 color toning */

  TInt aOutputVideoFormat;  /* video format of output movie */
  TInt iTimeIncrementResolution;  /* time increment resolution for current frame, if MPEG-4 */
  TInt vosHeaderSize;       /* VOS header size for current clip, if MPEG-4 */
  TInt streamMode;          /* current bitstream mode (data partition, regular, etc.) */

}vdeDecodeParamters_t;

/*
 * Classes 
 */
class MVideoRenderer;

/**
*  CVedH263Dec abstract class for H.263 decoder
*
*  @lib vedh263d
*  @since
*/
class CVedH263Dec : public CBase
    {

    public:  // Constants

        // post-filter type
        enum TPostFilter
        {
            ENoFilter = 0,
            EH263AnnexJFilter,
            EH263TMNFilter,
            ENokiaFilter
        };
        
        // color effect
        enum TColorEffect
        {
            EColorEffectNone = 0,
            EColorEffectBlackAndWhite = 1,
            EColorEffectToning = 2        // ColorToning feature
        }; 

        // error codes
        enum TErrorCode
        {
            EInternalAssertionFailure = -10000,
            EDecoderFailure = -10001,
            EDecoderNoIntra = -10002,
            EDecoderCorrupted = -10003
        };


    public:

        /**
        * Two-phased constructor.
        * @aFrameSize Indicates the size of the input frames which will be decoded
        * @aNumReferencePictures indicates the number of reference pictures 
        */

        IMPORT_C static CVedH263Dec* NewL(const TSize aFrameSize, const TInt aNumReferencePictures);

    public:  // new functions        

        /**
        * Sets the renderer object to be used
        * @since
        * @param aRenderer Pointer to the renderer object
        * @return void
        */
        virtual void SetRendererL(MVideoRenderer* aRenderer) = 0;

        /**
        * Sets the post-filter to be used in decoding
        * @since
        * @param aFilter filter to be used
        * @return void
        */
        virtual void SetPostFilterL(const TPostFilter aFilter) = 0;

        /**
        * Checks if the given frame contains valid data which can be displayed
        * @since
        * @param aFrame Frame to be checked
        * @return TBool ETrue if frame is valid
        */
        virtual TBool FrameValid(const TAny* aFrame) = 0;

        /**
        * Retrieves pointers to Y/U/V data for the given frame
        * @since
        * @param aFrame Pointer to the frame
        * @param aYFrame top-left corner of the Y frame 
        * @param aUFrame top-left corner of the U frame
        * @param aVFrame top-left corner of the V frame
        * @param aFrameSize
        * @return TInt error code
        */
        virtual TInt GetYUVBuffers(const TAny* aFrame, TUint8*& aYFrame, TUint8*& aUFrame,          
            TUint8*& aVFrame, TSize& aFrameSize) = 0;

        /**
        * Returns the given output frame to the decoder after it is not needed
        * @since
        * @param aFrame Frame to be returned
        * @return void
        */
        virtual void FrameRendered(const TAny* aFrame) = 0;

        /**
        * Gets a pointer the latest decoded frame (YUV concatenated)
        * @since
        * @param aFrame Frame to be returned
        * @return TUint8* pointer to the frame
        */
        virtual TUint8* GetYUVFrame() = 0;

        /**
        * Decodes / transcodes a compressed frame
        * @since
        * @param aInputBuffer Descriptor for the input bitstream buffer
        * @param aOutputBuffer Descriptor for the output bitstream buffer
        * @param aFirstFrame Flag (input/output) for the first frame, non-zero if the first frame is being decoded
        * @param aBytesDecoded Number of bytes that were decoded to produce the output frame                
        * @param aColorEffect Color effect to be applied 
        * @param aGetDecodedFrame ETrue if the output frame needs to be retrieved from the decoder later
        * @param aFrameOperation Operation to be performed: 1=EDecodeAndWrite,2=EDecodeNoWrite, 3=EWriteNoDecode
        * @param aTrP 
        * @param aTrD
        * @param aVideoClipNumber Number of the video clip
        * @param aSMSpeed Slow motion speed, max = 1000
        * @param aDecoderInfo pointer to editing param struct
		* @param aDataFormat Format of input/output data: 1 = H.263, 2 = MPEG-4
        * @return void
        */

        virtual void DecodeFrameL(const TPtrC8& aInputBuffer, TPtr8& aOutputBuffer,
                                  TBool& aFirstFrame, TInt& aBytesDecoded, 
                                  vdeDecodeParamters_t *aDecoderInfo) = 0;      

        virtual void DecodeFrameL(const TPtrC8& aInputBuffer, TPtr8& aOutputBuffer,
                                  TBool& aFirstFrame, TInt& aBytesDecoded, 
                                  const TColorEffect aColorEffect,
                                  const TBool aGetDecodedFrame, TInt aFrameOperation,
                                  TInt* aTrP, TInt* aTrD, TInt aVideoClipNumber, TInt aSMSpeed, 
                                  TInt aDataFormat) = 0;
        
        
        /**
        * Decodes a compressed frame
        * @since
        * @param aInputBuffer Descriptor for the input bitstream buffer
        * @param aFirstFrame Flag for the first frame, non-zero if the first frame is being decoded
        * @param aBytesDecoded Number of bytes that were decoded to produce the output frame        
		* @param aDataFormat Format of input/output data: 1 = H.263, 2 = MPEG-4
        * @return void
        */
        virtual void DecodeFrameL(const TPtrC8& aInputBuffer, TBool& aFirstFrame, TInt& aBytesDecoded, 
                    TInt aDataFormat) = 0;

        /**
        * Check the VOS header of one frame
        * @since
        * @param aInputBuffer Descriptor for the input bitstream buffer
        * @return TBool ETrue the buffer is changed
        */
        virtual TBool CheckVOSHeaderL(TPtrC8& aInputBuffer) = 0;

};

/**
*  MVideoRenderer Renderer interface for the H.263 decoder
*
*  @lib vedh263d
*  @since
*/

class MVideoRenderer
{

public:    
    virtual TInt RenderFrame(TAny* aFrame) = 0;

};


#endif

// End of File
