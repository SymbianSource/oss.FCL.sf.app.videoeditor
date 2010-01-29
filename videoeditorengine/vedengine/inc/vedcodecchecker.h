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
* Header for vedcodecchecker.cpp.
*
*/


#ifndef __VEDCODECCHECKER_H__
#define __VEDCODECCHECKER_H__


//  FORWARD DECLARATIONS
class CVedVideoClip;

//  CONSTANTS
//WVGA task
//const TInt KNumResolutions = 6;
//const TInt KNumCodecs = 14;
const TInt KNumResolutions = 7;
const TInt KNumCodecs = 18;

enum TResolution
    {
    EResolutionSubQCIF = 0,
    EResolutionQCIF,
    EResolutionCIF,
    EResolutionQVGA,
    EResolutionVGA16By9,
    EResolutionVGA,
    EResolutionWVGA,
    EResolutionUnsupported
    };
    
enum TCodec
    {
    ECodecH263BPLevel10 = 0,
    ECodecH263BPLevel45,
    ECodecMPEG4VSPLevel0,
    ECodecMPEG4VSPLevel0B,
    ECodecMPEG4VSPLevel1,
    ECodecMPEG4VSPLevel2,   
    ECodecMPEG4VSPLevel3,
    ECodecMPEG4VSPLevel4,
    ECodecAVCBPLevel1,
    ECodecAVCBPLevel1B,
    ECodecAVCBPLevel1_1,
    ECodecAVCBPLevel1_2,    
    ECodecAVCBPLevel1_3,    
    ECodecAVCBPLevel2,
    //WVGA task
    ECodecAVCBPLevel2_1,
    ECodecAVCBPLevel2_2,    
    ECodecAVCBPLevel3,    
    ECodecAVCBPLevel3_1,   
    ECodecUnsupported
    };    
    
class CVedCodecChecker : public CBase
    {
  
public:  // New functions  

    /* Constructors. */
    static CVedCodecChecker* NewL();
    static CVedCodecChecker* NewLC();
	
    /* Destructor. */
    virtual ~CVedCodecChecker();

    /**
     * Returns whether the given input clip is supported, 
     * i.e. if it can be decoded or not
     *
     * @param aClip Video clip
     * @return TBool Is supported ?
     */    
    TBool IsSupportedInputClipL(CVedVideoClip *aClip);
    
    /**
     * Returns whether the given output format is supported, 
     * i.e. if it can be encoded or not
     *
     * @param aMimeType Codec MIME type
     * @param aResolution Desired resolution
     * @return TBool Is supported ?
     */    
    TBool IsSupportedOutputFormatL(const TPtrC8& aMimeType, TSize aResolution);        
    
private:  // Private methods

    /**
    * By default Symbian OS constructor is private.
    */
    void ConstructL();
    
    /**
    * C++ default constructor
    */
    CVedCodecChecker();
    
    /**
     * Determines supported input formats using CTRTranscoder     
     */
    void GetSupportedInputFormatsL();
    
    /**
     * Determines supported output formats using CTRTranscoder     
     */
    void GetSupportedOutputFormatsL();
       
    /**
     * Maps resolution from TSize to TResolution
     *
     * @param aResolution Resolution to map
     * @return TResolution
     */ 
    TResolution MapResolution(TSize aResolution);
    
    /**
     * Maps profile-level-id to TCodec
     *
     * @param aProfileLevelId id to map
     * @return TCodec Codec
     */
    TCodec MapProfileLevelId(TInt aProfileLevelId);        
    
    /**
     * Parse codec MIME type
     *
     * @param aMimeType MIME to parse
     * @param aResolution resolution to be used
     * @return TCodec Codec
     */
    TCodec ParseMimeType(const TPtrC8& aMimeType, TSize aResolution);
    
     /**
     * Maps AVC level to TCodec
     *
     * @param aLevel Level to map
     * @return TCodec Codec
     */
    TCodec MapAVCLevel(TInt aLevel);
    
private:  // Data
    
    // table of supported input resolutions for each codec
    TBool* iInputCodecsAndResolutions[KNumCodecs];
    
    // table of supported output resolutions for each codec
    TBool* iOutputCodecsAndResolutions[KNumCodecs];
    
    // have output formats been checked ??
    TBool iOutputFormatsChecked;

};

#endif // __VEDCODECCHECKER_H__


// End of file
