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
* Definition for CMPEG4Transcoder, a class for MPEG4 transcoder 
* working at video packet level or MB level.
*
*/



#ifndef     __VMPEG4TRANSCODER_H__
#define     __VMPEG4TRANSCODER_H__


/* 
 * Includes (header files are copied from core_mpeg.cpp)
 */
#include <e32base.h>

#include <vedcommon.h>
#include "h263dConfig.h"
#include "vdc263.h"
#include "core.h"
#include "debug.h"
#include "decblock.h" /* for dblFree and dblLoad */
#include "decvp_mpeg.h"
#include "h263dapi.h" /* for H263D_BC_MUX_MODE_SEPARATE_CHANNEL and H263D_ERD_ */
#include "vdeimb.h"
#include "viddemux.h"
#include "vdxint.h"
#include "vde.h"
#include "vdemain.h"
#include "biblin.h"
#include "mpegcons.h"
#include "h263dmai.h"
#include "decpich.h"
#include "decmbdct.h"
#include "common.h"
#include "sync.h"
#include "vdcaic.h"
#include "zigzag.h"
#include "debug.h"

#ifndef VDTASSERT
// An assertion macro wrapper to clean up the code a bit
#define VDTASSERT(x) __ASSERT_DEBUG(x, User::Panic(_L("CMPEG4Transcoder"), EInternalAssertionFailure)) 
#endif



/* Get information about video bitstream */
int vdtGetVideoBitstreamInfo(bibBuffer_t *inBuffer, vdeDecodeParamters_t *aInfoOut, int *aByteIndex, int *aBitIndex);

/* Finds the error resilience bit in MPEG-4 VOL and change it, if necessary, to make it Regular Resynchronization mode */
TBool vdtChangeVosHeaderRegResyncL(TPtrC8& aInputBuffer, TUint aBufferSize);


#define DefaultTimeIncResolution 3000/1001

// unify error codes
#define TX_OK H263D_OK
#define TX_ERR H263D_ERROR

/*
* CopyEditVop
*    
*
* Parameters:
*    hInstance                  instance handle
*
* Function:
* This function copies the VOP header with edited time stamps of the VOP.
*
* Params
*   @param hInstance is handle instance  itself  
*   @param aNrBytesToSkip  number of bytes to jump over in the beginning of output buffer (e.g. MPEG4 VOS header)
*   @param inBuffer is the input buffer
*   @param aDecoderInfo structure with decode params
*/

int CopyEditVop(vdeHInstance_t hInstance, int aNrOfBytesToSkip, bibBuffer_t * inBuffer, vdeDecodeParamters_t *aDecoderInfo);




/*
* CopyEditVideoPacket
*    
*
* Parameters:
*
* Function:
*    This function copies the video packet with edited time stamps, if HEC is enabled
*
* Returns:
*    Success or failure
*/
int CopyEditVideoPacket(bibBuffer_t* inBuffer, 
                        bibBuffer_t* outBuffer, 
                        bibBufferEdit_t* aBufEdit, 
                        vdcInstance_t * vdcTemp,
                        vdeDecodeParamters_t *aDecoderInfo, 
                        vdxVopHeader_t* vopheader, 
                        int* aSncCode, 
                        int* startByteIndex, 
                        int* startBitIndex);





/*
 * Classes
 */

/* General transcoding class
 *   contains all necessary transcoding information
 *   accessible from various points in the decoder module 
 */
class CMPEG4Transcoder : public CBase
{
public:
    
  static CMPEG4Transcoder* NewL (const vdeInstance_t *aVDEInstance, bibBuffer_t *aInBuffer, bibBuffer_t *aOutBuffer);

  /* Destructor */
    ~CMPEG4Transcoder();

    /* Indicates whether we need to do  MPEG4 bitstream transcoding */
    int SetTranscoding(vdeDecodeParamters_t *aDecoderInfo);

    /* Copy the VOP Header to output buffer */
    void VOPHeaderEnded(int aStartByteIndex, int aStartBitIndex, 
      int aQuant, int aPicType, int aFrameNum, int aVOPNotCoded);
  /* One VOP ended */
    void VOPEnded();
  /* Record the position before one video packet is processed */
    void BeginOneVideoPacket(dvpVPInParam_t *aVPin);
    /* Record the position before the content of the video packet is processed (called after retreiving the video packet) */
    void AfterVideoPacketHeader(dvpVPInOutParam_t *aVPInfo);
    /* Called after one video packet's contents are retrieved */
    void OneVPEnded();
  /* Add one IMB instance for I macroblock */
    void OneIMBDataStartedDataPartitioned(vdxIMBListItem_t *aMBInstance, dlst_t *aMBList, int aCurrMBNumInVP, int aMBNum);
    /* Add one PMB instance for P macroblock */
    void OnePMBDataStartedDataPartitioned(vdxPMBListItem_t *aMBInstance, dlst_t *aMBList, int aCurrMBNumInVP, int aMBNum);
    /* Records the position of vop_time_increment_resolution bit */
    void MPEG4TimerResolution(int aStartByteIndex, int aStartBitIndex);
    /* Creates a new H263 macroblock */
    int ConstructH263MBData(dmdPParam_t *aParam, int aNewMCBPCLen, int aNewMCBPC);
    /* Transcode one H263 MB to one MPEG4 MB */
    void H263ToMPEG4MBData(int aNewMCBPCLen, int aNewMCBPC);
    /* fills the reconstructed DCAC values for INTRA block */
    void AddOneBlockDCACrecon(int aIndex, int *aDCAC);
    /* Record MB parameters before the content of the MB is processed (called after retreiving the MB layer) */
    void AfterMBLayer(int aUpdatedQp);
    /* returns first frame QP */
    inline int GetRefQP() { return iRefQuant; }


    /* returns 1 if we need the decoded frame */
    int  NeedDecodedYUVFrame();
    /* Record the position before one MB is processed */
    void BeginOneMB(int aMBNum);
    /* Record the position before one block in MB is processed */
    void BeginOneBlock(int aIndex);
  /* Start one IMB (called after the MB header is read) */
    void OneIMBDataStarted(vdxIMBListItem_t *aMBInstance);
  /* Start one PMB (called after the MB header is read) */
    void OnePMBDataStarted(vdxPMBListItem_t *aMBInstance);
  /* Input one block data to current MB */
    void AddOneBlockDataToMB(int aBlockIndex, int *aBlockData);
    /* Indicates if escape vlc coding is used in one block */
    void H263EscapeCoding(int aIndex, int fEscapeCodeUsed);
  /* Records the position of resnc_marker_disable bit */
    void ErrorResilienceInfo(vdxVolHeader_t *header, int aByte, int aBit);
  /* Constructs the VOS header */
    void ConstructVOSHeader(int aMPEG4, vdeDecodeParamters_t *aDecoderInfo);
  /* Called after one H263 GOB or slice header is parsed */
    void H263GOBSliceHeaderEnded(vdxGOBHeader_t *aH263GOBHeader, vdxSliceHeader_t *aH263SliceHeader);   
  /* Called after one H263 picture header is parsed */
    int H263PictureHeaderEnded(dphOutParam_t *aH263PicHeader, dphInOutParam_t *aInfo);
  /* Called before one H.63 GOB or slice header is parsed */
    void H263GOBSliceHeaderBegin();
  /* Called after one H263 GOB or Slice ends */
    void H263OneGOBSliceEnded(int nextExpectedMBNum);
  /* Called after one GOB (slice) that has GOB header (except the first GOB) ends */
    void H263OneGOBSliceWithHeaderEnded();
    /* Transcodes one macroblock (may include dequantization, requantization, and re-encoding) */
    int TranscodingOneMB(dmdPParam_t *aParam);

private:
		/* Constructors */
    CMPEG4Transcoder(const vdeInstance_t *aVDEInstance, bibBuffer_t *aInBuffer, bibBuffer_t *aOutBuffer);

        /*Constructl to allocate necessary resources*/
        void ConstructL();

    // main functions to perform  transcoding for one macroblock
    /* Transcode MB - rearranges data partitioned bitstream data to regular */
    void ConstructRegularMPEG4MBData(int aNewMCBPCLen, int aNewMCBPC);
  /* Resets the DCs for U/V blocks in intra MB */
    void ResetMPEG4IntraDcUV();
  /* Recontruct IMB partitions for color effect when we are not doing format transcoding */
    void ReconstructIMBPartitions();
  /* Recontruct PMB partitions for color effect when we are not doing format transcoding */
    void ReconstructPMBPartitions();
  /* Evaluate Chrominance DC Scaler from QP for MPEG4 */
    int GetMpeg4DcScalerUV(int aQP);
  /* Evaluate IntraDC Coefficients for U,V blocks for MPEG4 */
    void GetMPEG4IntraDcCoeffUV(TInt aValue, TInt& aSize, TInt& aSizeCode, 
      TInt& aSizeCodeLength, TInt& aValueCode, TInt& aValueCodeLength);



private:

	    /* Output data buffer */
    bibBuffer_t *iOutBuffer;
    /* Operation modes */
    TVedVideoBitstreamMode iBitStreamMode;

    /* 
    When we do mode translation for MPEG4, the target mode may be MPEG4Resyn or H263
       but for H263, when we do mode translation, the target mode is always MPG4Resyn
    */

/* Member variables */

    /* Indicates the direction for MPEG4 mode translation */
    int iTargetFormat; 

    /* Input data buffer */
    bibBuffer_t *iInBuffer;

    /* Indicates if we need to do transcoding for current MB */
    int iDoTranscoding;

    /* Indicates if we need to do MPEG4 bitstream conversion */
    int iDoModeTranscoding;

    /* Special Effect (Black and White) paramter */
    int iColorEffect;
    
    /* color tone U,V value parameter */
    TInt iColorToneU;
    TInt iColorToneV;

    /* Information we know about this frame and MB */

    /* current VOP coding type */
    int iVopCodingType;

    /* number of MBs in one VOP */
    int iNumMBsInOneVOP;

    /* indicates if stuffing bits have been used */  
    int8 iStuffingBitsUsed;


    /* Only valid with resync_marker
       Information includes:
            MB number, quant_scale, StartByteIndex, StartBitIndex ...
    */
    /* Contains CurMBNum, quant, frame number, etc */
    dvpVPInOutParam_t *iCurVPInOut; 
    /* Contains picture type, etc */
    dvpVPInParam_t *iCurVPIn; 
    /* VOL header */
    vdxVolHeader_t iVOLHeader;

    /* position pointers at different levels */
    int iVPStartByteIndex, iVPStartBitIndex;
    int iVPHeaderEndByteIndex, iVPHeaderEndBitIndex;
    int iTimeResolutionByteIndex, iTimeResolutionBitIndex;
    /* Array to store the DCT data */
    int iDCTBlockData[BLOCK_COEFF_SIZE * 6];                 /* YUV components */
    int **iH263DCData;           /* Intra DC matrix for H263 -> MPEG4 transcoding */

    dlst_t *iMBList;
    tMBInfo* h263mbi;     /* one frame MB information for MPEG4 to H263 transcoding */
    int iPreQuant;        /* last QUANT, used in MPEG4 to H263 transcoding */
    
    /* used for color toning for MPEG4 */
    u_char *iMBType;       /* array indicating MB type for all MBs in VOP */
    int iRefQuant;         /* reference QP derived from 1st VOP of MPEG4 */
    int iCurQuant;         /* current QP */
    int iDcScaler;         /* DC Scaler for iRefQuant */
                        

    int *iH263MBVPNum;    /* matrix recording the video packet number for each MB */

    int iShortHeaderEndByteIndex ,  iShortHeaderEndBitIndex;

    /* iMBStartByteIndex, iMBStartBitIndex also record the postion 
       right after the VP header when video packet is used
    */
    int iMBStartByteIndex, iMBStartBitIndex;

    /* Positions of each block data including INTRA DC if it exists 
       except for data partioning, in which it only indicates the ACs or DCTs
    */
    int iBlockStartByteIndex[6], iBlockStartBitIndex[6];
    int iBlockEndByteIndex[6], iBlockEndBitIndex[6];
    int iErrorResilienceStartByteIndex, iErrorResilienceStartBitIndex;

    /*Added for Time Res 30000 */
    int iOutputMpeg4TimeIncResolution;

    /* Processing data member */
    const vdeInstance_t *iVDEInstance;

    /* Instance for current processing MB */
    vdxIMBListItem_t *iCurIMBinstance;
    vdxPMBListItem_t *iCurPMBinstance; 
    
    /* Temporary edit buffer */
    bibBufferEdit_t      bufEdit;
    
    /* Current and last processed MB */
    int iCurMBNum;

    /* Last output MB number */
    int iLastMBNum;

    /* current MB number in current video packet */
    int iCurMBNumInVP;

    /* MB coding type: INTRA or INTER */
    int iMBCodingType;

  /* Specific members only for H.263! */
    int iGOBSliceStartByteIndex;
    int iGOBSliceStartBitIndex;
    int iGOBSliceHeaderEndByteIndex;
    int iGOBSliceHeaderEndBitIndex;

  /* Index of video packet in MPEG4 */
    int iVideoPacketNumInMPEG4;  
  /* Number of MBs in picture width */
    int iMBsinWidth;  
  /* Number of MBs in one GOB */
    int iNumMBsInGOB; 
  /* Indicates if its the first frame */
    unsigned char fFirstFrameInH263;
  /* Indicates if escape vlc coding is used */
  int iEscapeCodeUsed[6]; 

};



#endif      /* __VMPEG4TRANSCODER_H__ */
            
