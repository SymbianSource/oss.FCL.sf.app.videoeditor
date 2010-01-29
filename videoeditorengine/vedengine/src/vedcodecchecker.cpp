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
*
*/


// Include Files
#include <mp4lib.h>
#include "ctrtranscoder.h"
#include "ctrtranscoderobserver.h"
#include "VedVideoClip.h"
#include "vedvideosettings.h"
#include "vedvolreader.h"
#include "vedcodecchecker.h"
#include "vedavcedit.h"

// Constants
const TUint KVOLHeaderBufferSize = 256;
const TUint KAVCDCRBufferSize = 16384;
const TUint KSubQCIFWidth = 128;
const TUint KQCIFWidth = 176;
const TUint KCIFWidth = 352;
const TUint KQVGAWidth = 320;
const TUint KVGAWidth = 640;
//WVGA task
const TUint KWVGAWidth = 864;

// An assertion macro wrapper to clean up the code a bit
#define CCASSERT(x) __ASSERT_DEBUG(x, User::Panic(_L("CVedCodecChecker"), -5000))

// Print macro
#ifdef _DEBUG
#include <e32svr.h>
#define PRINT(x) RDebug::Print x
#else
#define PRINT(x)
#endif

// Near-dummy observer class for temporary transcoder instance. In practice is only used to provide input framerate
// to the transcoder
class CTrObs : public CBase, public MTRTranscoderObserver
    {
public:
    /* Constructor & destructor */
    
    inline CTrObs(TReal aFrameRate) : iInputFrameRate(aFrameRate) 
        {
        };
    inline ~CTrObs()
        {
        };

    // Dummy methods from MTRTranscoderObserver, just used to complete the observer class
    inline void MtroInitializeComplete(TInt /*aError*/)
        {
        };
    inline void MtroFatalError(TInt /*aError*/)
        {
        };
    inline void MtroReturnCodedBuffer(CCMRMediaBuffer* /*aBuffer*/) 
        {
        };
    // method to provide clip input framerate to transcoder
    inline void MtroSetInputFrameRate(TReal& aRate)
        {
        aRate = iInputFrameRate;
        };
    inline void MtroAsyncStopComplete()
        {
        };
        
    inline void MtroSuspend()
        {
        };
        
    inline void MtroResume()
        {
        };
        
private:// data

        // clip input framerate (fps)
        TReal iInputFrameRate;
    
    };


// ================= MEMBER FUNCTIONS =======================

// -----------------------------------------------------------------------------
// CVedCodecChecker::NewL
// Two-phased constructor.
// -----------------------------------------------------------------------------
//
CVedCodecChecker* CVedCodecChecker::NewL() 
    {
    PRINT(_L("CVedCodecChecker::NewL in"));
    
    CVedCodecChecker* self = NewLC();
    CleanupStack::Pop(self);
    
    PRINT(_L("CVedCodecChecker::NewL out"));
    return self;
    }

CVedCodecChecker* CVedCodecChecker::NewLC()
    {
    PRINT(_L("CVedCodecChecker::NewLC in"));
    
    CVedCodecChecker* self = new (ELeave) CVedCodecChecker();
    CleanupStack::PushL(self);
    self->ConstructL();

    PRINT(_L("CVedCodecChecker::NewLC out"));
    return self;
    }
    
// -----------------------------------------------------------------------------
// CVedCodecChecker::CVedCodecChecker
// C++ default constructor can NOT contain any code, that
// might leave.
// -----------------------------------------------------------------------------
//
CVedCodecChecker::CVedCodecChecker() : iOutputFormatsChecked(EFalse)
    {    
    }
    
// -----------------------------------------------------------------------------
// CSizeEstimate::~CSizeEstimate
// Destructor.
// -----------------------------------------------------------------------------
//
CVedCodecChecker::~CVedCodecChecker()
    {

    // free all memory
    for (TInt i = 0; i < KNumCodecs; i++)
    {
        if (iInputCodecsAndResolutions[i] != 0)
            delete [] iInputCodecsAndResolutions[i];                    
        
        iInputCodecsAndResolutions[i] = 0;
        
        if (iOutputCodecsAndResolutions[i] !=  0)
            delete [] iOutputCodecsAndResolutions[i];                    
        
        iOutputCodecsAndResolutions[i] = 0;
    }
    
        
    }

// -----------------------------------------------------------------------------
// CSizeEstimate::ConstructL
// Symbian 2nd phase constructor can leave.
// -----------------------------------------------------------------------------
//
void CVedCodecChecker::ConstructL()
    {
    
    PRINT(_L("CVedCodecChecker::ConstructL in"));
    
    TInt i;
    
    // reset pointer tables
    for (i = 0; i < KNumCodecs; i++)
    {
        iInputCodecsAndResolutions[i] = 0;
        iOutputCodecsAndResolutions[i] = 0;
    }
    
    // allocate resolution tables
    for (i = 0; i < KNumCodecs; i++)
    {                        
        TBool* temp =  new ( ELeave ) TBool[KNumResolutions];
        for (TInt j = 0; j < KNumResolutions; j++)
            temp[j] = 0;            
        
        iInputCodecsAndResolutions[i] = temp;        
        
        temp =  new ( ELeave ) TBool[KNumResolutions];
        for (TInt j = 0; j < KNumResolutions; j++) 
            temp[j] = 0;            
        
        iOutputCodecsAndResolutions[i] = temp;
    }
        
    // check supported input codecs / resolutions
    GetSupportedInputFormatsL();
    
    PRINT(_L("CVedCodecChecker::ConstructL out"));
        
    }

// -----------------------------------------------------------------------------
// CVedCodecChecker::GetSupportedInputFormatsL
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//    
void CVedCodecChecker::GetSupportedInputFormatsL()
    {        

    PRINT(_L("CVedCodecChecker::GetSupportedInputFormatsL in"));

    const TSize KResolutions[KNumResolutions] = { KVedResolutionSubQCIF, 
                                                  KVedResolutionQCIF, 
                                                  KVedResolutionCIF, 
                                                  KVedResolutionQVGA, 
                                                  KVedResolutionVGA16By9,
                                                  KVedResolutionVGA,
                                                  //WVGA task
                                                  KVedResolutionWVGA };

    const TPtrC8 KCodecs[KNumCodecs] = { _L8("video/H263-2000; profile=0; level=10"),
                                         _L8("video/H263-2000; profile=0; level=45"),
                                         _L8("video/mp4v-es; profile-level-id=8"),
                                         _L8("video/mp4v-es; profile-level-id=9"),
                                         _L8("video/mp4v-es; profile-level-id=1"),
                                         _L8("video/mp4v-es; profile-level-id=2"),
                                         _L8("video/mp4v-es; profile-level-id=3"),
                                         _L8("video/mp4v-es; profile-level-id=4"),                                         
                                         _L8("video/H264; profile-level-id=42800A"),
                                         _L8("video/H264; profile-level-id=42900B"),
                                         _L8("video/H264; profile-level-id=42800B"),
                                         _L8("video/H264; profile-level-id=42800C"), 
                                         _L8("video/H264; profile-level-id=42800D"), 
                                         _L8("video/H264; profile-level-id=428014"), 
                                         //WVGA task
                                         _L8("video/H264; profile-level-id=428015"),
                                         _L8("video/H264; profile-level-id=428016"), 
                                         _L8("video/H264; profile-level-id=42801E"), 
                                         _L8("video/H264; profile-level-id=42801F")  };   

    TTRVideoFormat inputFormat;
    TTRVideoFormat outputFormat;      
    
    inputFormat.iDataType = CTRTranscoder::ETRDuCodedPicture;
    outputFormat.iDataType = CTRTranscoder::ETRYuvRawData420;        
    
    for (TInt i = 0; i < KNumCodecs; i++)
    {    
    
        PRINT((_L("GetSupportedInputFormatsL - testing codec %d"), i));
    
        // create temporary transcoder observer object, with input framerate as parameter, since it is asked by the transcoder
        CTrObs* tmpObs = new (ELeave) CTrObs(15.0);
        CleanupStack::PushL(tmpObs);
    
        // create temporary transcoder instance
        CTRTranscoder* tmpTranscoder = CTRTranscoder::NewL(*tmpObs);
        CleanupStack::PushL(tmpTranscoder);
    
        // check if codec supported at all    
        if ( tmpTranscoder->SupportsInputVideoFormat(KCodecs[i]) )
        {
            TInt error = KErrNone;
            // check all resolutions
            for (TInt j = 0; j < KNumResolutions; j++)
            {                            
            
                if ( (i < ECodecMPEG4VSPLevel0) && (j > EResolutionQCIF) )
                {
                    // Do not allow larger resolutions than QCIF for H.263
                    CleanupStack::PopAndDestroy(tmpTranscoder);
                    CleanupStack::PopAndDestroy(tmpObs);     
                    PRINT((_L("GetSupportedInputFormatsL - break")));
                    break;
                }
                
                PRINT((_L("GetSupportedInputFormatsL - testing (%d, %d)"), i, j));

                inputFormat.iSize = outputFormat.iSize = KResolutions[j];
                
                TRAP( error, tmpTranscoder->OpenL(reinterpret_cast<MCMRMediaSink*>(1),//the sink will not be used, hence this is acceptable 
                                     CTRTranscoder::EDecoding,
                                     KCodecs[i],
                                     KNullDesC8,
                                     inputFormat, 
                                     outputFormat, 
                                     EFalse ) );
                                     
                if (error == KErrNone)
                {
                    PRINT((_L("GetSupportedInputFormatsL - (%d, %d) supported"), i, j));
                    iInputCodecsAndResolutions[i][j] = ETrue;
                } 
                else if (error == KErrNotSupported)
                {
                    PRINT((_L("GetSupportedInputFormatsL - (%d, %d) not supported"), i, j));
                    iInputCodecsAndResolutions[i][j] = EFalse;                    
                } 
                else
                    User::Leave(error);
                
                CleanupStack::PopAndDestroy(tmpTranscoder);
                CleanupStack::PopAndDestroy(tmpObs);     
                
                if ( j < (KNumResolutions - 1) )
                {                                    
                    tmpObs = new (ELeave) CTrObs(15.0);
                    CleanupStack::PushL(tmpObs);
                                
                    // create temporary transcoder instance
                    tmpTranscoder = CTRTranscoder::NewL(*tmpObs);
                    CleanupStack::PushL(tmpTranscoder);  
                }
            }
        } 
        else
        {
            // all resolutions unsupported
            for (TInt j=0; j < KNumResolutions; j++)
            {            
                iInputCodecsAndResolutions[i][j] = EFalse;
            }
            
            CleanupStack::PopAndDestroy(tmpTranscoder);
            CleanupStack::PopAndDestroy(tmpObs);
                 
        }    
    }   
    
    PRINT(_L("CVedCodecChecker::GetSupportedInputFormatsL out"));

    }

// -----------------------------------------------------------------------------
// CVedCodecChecker::GetSupportedOutputFormatsL
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//    
void CVedCodecChecker::GetSupportedOutputFormatsL()
    {        
    
    PRINT(_L("CVedCodecChecker::GetSupportedOutputFormatsL in"));

    const TSize KResolutions[KNumResolutions] = { KVedResolutionSubQCIF, 
                                                  KVedResolutionQCIF, 
                                                  KVedResolutionCIF, 
                                                  KVedResolutionQVGA, 
                                                  KVedResolutionVGA16By9,
                                                  KVedResolutionVGA };

    const TPtrC8 KCodecs[KNumCodecs] = { _L8("video/H263-2000; profile=0; level=10"),
                                         _L8("video/H263-2000; profile=0; level=45"),
                                         _L8("video/mp4v-es; profile-level-id=8"),
                                         _L8("video/mp4v-es; profile-level-id=9"),
                                         _L8("video/mp4v-es; profile-level-id=1"),
                                         _L8("video/mp4v-es; profile-level-id=2"),
                                         _L8("video/mp4v-es; profile-level-id=3"),
                                         _L8("video/mp4v-es; profile-level-id=4"),                                          
                                         _L8("video/H264; profile-level-id=42800A"),
                                         _L8("video/H264; profile-level-id=42900B"),
                                         _L8("video/H264; profile-level-id=42800B"),
                                         _L8("video/H264; profile-level-id=42800C"), 

                                         //WVGA task
                                         //(TPtrC8&)KNullDesC8,  // level 1.3 not supported in output
                                         //(TPtrC8&)KNullDesC8,   // level 2 not supported in output  
                                         _L8("video/H264; profile-level-id=42800D"),
                                         _L8("video/H264; profile-level-id=428014"),                                          
                                         _L8("video/H264; profile-level-id=428015"),
                                         _L8("video/H264; profile-level-id=428016"), 
                                         _L8("video/H264; profile-level-id=42801E"), 
                                         _L8("video/H264; profile-level-id=42801F")                                      
                                       }; 
                                       
                                       

    TTRVideoFormat inputFormat;
    TTRVideoFormat outputFormat;
    
    inputFormat.iDataType = CTRTranscoder::ETRYuvRawData420;
    outputFormat.iDataType = CTRTranscoder::ETRDuCodedPicture;
    
    for (TInt i = 0; i < KNumCodecs; i++)
    {        
        PRINT((_L("GetSupportedOutputFormatsL - testing codec %d"), i));
    
        // create temporary transcoder observer object, with input framerate as parameter, since it is asked by the transcoder
        CTrObs* tmpObs = new (ELeave) CTrObs(15.0);
        CleanupStack::PushL(tmpObs);
    
        // create temporary transcoder instance
        CTRTranscoder* tmpTranscoder = CTRTranscoder::NewL(*tmpObs);
        CleanupStack::PushL(tmpTranscoder);
        
        //WVGA task
        // AVC levels 3.1 and higher are not supported in output
        if (i >= ECodecAVCBPLevel3_1)
        {
            CleanupStack::PopAndDestroy(tmpTranscoder);
            CleanupStack::PopAndDestroy(tmpObs);
            PRINT((_L("GetSupportedOutputFormatsL - break AVC check")));   
            break;
        }
       
    
        // check if codec supported at all    
        if ( tmpTranscoder->SupportsOutputVideoFormat(KCodecs[i]) )
        {
            TInt error = KErrNone;
            // check all resolutions
            for (TInt j = 0; j < KNumResolutions; j++)
            {
            
                if ( (i < ECodecMPEG4VSPLevel0) && (j > EResolutionQCIF) )
                {
                    // Do not allow larger resolutions than QCIF for H.263
                    CleanupStack::PopAndDestroy(tmpTranscoder);
                    CleanupStack::PopAndDestroy(tmpObs);     
                    PRINT((_L("GetSupportedOutputFormatsL - break")));
                    break;
                }
                
                PRINT((_L("GetSupportedOutputFormatsL - testing (%d, %d)"), i, j));            

                inputFormat.iSize = outputFormat.iSize = KResolutions[j];
                
                TRAP( error, tmpTranscoder->OpenL(reinterpret_cast<MCMRMediaSink*>(1),//the sink will not be used, hence this is acceptable 
                                     CTRTranscoder::EEncoding,
                                     KNullDesC8,
                                     KCodecs[i],                                     
                                     inputFormat, 
                                     outputFormat, 
                                     EFalse ) );
                                     
                if (error == KErrNone)
                {
                    PRINT((_L("GetSupportedOutputFormatsL - (%d, %d) supported"), i, j));
                    iOutputCodecsAndResolutions[i][j] = ETrue;
                } 
                else if (error == KErrNotSupported)
                {
                    PRINT((_L("GetSupportedOutputFormatsL - (%d, %d) not supported"), i, j));
                    iOutputCodecsAndResolutions[i][j] = EFalse;                    
                } 
                else
                    User::Leave(error);
                
                CleanupStack::PopAndDestroy(tmpTranscoder);
                CleanupStack::PopAndDestroy(tmpObs);     
                
                if ( j < (KNumResolutions - 1) )
                {                                    
                    tmpObs = new (ELeave) CTrObs(15.0);
                    CleanupStack::PushL(tmpObs);
                                
                    // create temporary transcoder instance
                    tmpTranscoder = CTRTranscoder::NewL(*tmpObs);
                    CleanupStack::PushL(tmpTranscoder);  
                }
            }
        } 
        else
        {
            // all resolutions unsupported
            for (TInt j=0; j < KNumResolutions; j++)
            {            
                iOutputCodecsAndResolutions[i][j] = EFalse;
            }
            
            CleanupStack::PopAndDestroy(tmpTranscoder);
            CleanupStack::PopAndDestroy(tmpObs);
                 
        }    
    }   
    
    PRINT(_L("CVedCodecChecker::GetSupportedOutputFormatsL out"));

    }


// -----------------------------------------------------------------------------
// CVedCodecChecker::IsSupportedInputClip
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//    
TBool CVedCodecChecker::IsSupportedInputClipL(CVedVideoClip* aClip)
    {
    
    PRINT(_L("CVedCodecChecker::IsSupportedInputClipL in"));

    if ( (aClip->Info()->VideoType() != EVedVideoTypeH263Profile0Level10) && 
         (aClip->Info()->VideoType() != EVedVideoTypeH263Profile0Level45) && 
         (aClip->Info()->VideoType() != EVedVideoTypeMPEG4SimpleProfile) &&
         (aClip->Info()->VideoType() != EVedVideoTypeAVCBaselineProfile)
        )
    {
        return KErrNotSupported;
    }
        
    TResolution resolution = MapResolution( aClip->Info()->Resolution() );

    if ( resolution == EResolutionUnsupported )
        return EFalse;
    
    if ( aClip->Info()->VideoType() == EVedVideoTypeH263Profile0Level10 )
    {
        return iInputCodecsAndResolutions[ECodecH263BPLevel10][resolution];
    } 
    
    if ( aClip->Info()->VideoType() == EVedVideoTypeH263Profile0Level45 )
    {
        return iInputCodecsAndResolutions[ECodecH263BPLevel45][resolution];
    }        
    
#ifndef VIDEOEDITORENGINE_AVC_EDITING        
    if ( aClip->Info()->VideoType() == EVedVideoTypeAVCBaselineProfile )
    {
        return EFalse;
    }        
#endif

    // clip is MPEG-4 or AVC
            
    // create parser to fetch codec specific info    
    MP4Handle mp4Handle = 0;
    MP4Err mp4Error;
    if (!aClip->Info()->FileHandle())
    {        
        TBuf<258> tempFileName(aClip->Info()->FileName());
        tempFileName.ZeroTerminate();			
                
        MP4FileName name = reinterpret_cast<MP4FileName>( const_cast<TUint16*>(tempFileName.Ptr()) );
        mp4Error = MP4ParseOpen(&mp4Handle, name);
    } 
    else
    {
        mp4Error = MP4ParseOpenFileHandle(&mp4Handle, aClip->Info()->FileHandle());
    }
    
    if (mp4Error != MP4_OK)
    {
        if (mp4Handle)
            MP4ParseClose(mp4Handle);
        mp4Handle = 0;
    
        if (mp4Error == MP4_OUT_OF_MEMORY)
            User::Leave(KErrNoMemory);
        else
            User::Leave(KErrGeneral);
    }
            
    TInt bufSize = 0;
    
    if (aClip->Info()->VideoType() == EVedVideoTypeMPEG4SimpleProfile)
        bufSize = KVOLHeaderBufferSize;
    else
        bufSize = KAVCDCRBufferSize;
        
    HBufC8* tmpBuffer = (HBufC8*) HBufC8::NewLC(bufSize);
    
    TPtr8 tmpPtr = tmpBuffer->Des();
    mp4_u32 infoSize = 0;
    
    // get info
    mp4Error = MP4ParseReadVideoDecoderSpecificInfo( mp4Handle, 
                                                    (mp4_u8*)(tmpPtr.Ptr()),
                                                    bufSize,
                                                    &infoSize );
                                                    
    MP4ParseClose(mp4Handle);                                                    
                                                    
    if ( mp4Error != MP4_OK )
    {
        User::Leave(KErrGeneral);
    }   

    tmpPtr.SetLength(infoSize);
    
    TCodec codec = ECodecUnsupported;
    
    if (aClip->Info()->VideoType() == EVedVideoTypeMPEG4SimpleProfile)
    {
        // Parse profile-level-id
        CVedVolReader* tmpVolReader = CVedVolReader::NewL();
        CleanupStack::PushL(tmpVolReader);

        tmpVolReader->ParseVolHeaderL(tmpPtr);

        TInt profileLevelId = tmpVolReader->ProfileLevelId();
        
        codec = MapProfileLevelId(profileLevelId);
    }     
    
#ifdef VIDEOEDITORENGINE_AVC_EDITING    
    else    
    {
        CVedAVCEdit* tmpAvcEdit = CVedAVCEdit::NewL();
        CleanupStack::PushL(tmpAvcEdit);
    
        // this leaves with KErrNotSupported if clip is not supported
        tmpAvcEdit->SaveAVCDecoderConfigurationRecordL(tmpPtr, EFalse);
    
        // Parse level    
        TInt level = 0;
        User::LeaveIfError( tmpAvcEdit->GetLevel(tmpPtr, level) );
        
        codec = MapAVCLevel(level);
    }
#endif
    
    CleanupStack::PopAndDestroy(2);
	
    if (codec == ECodecUnsupported)
        return EFalse;

    PRINT(_L("CVedCodecChecker::IsSupportedInputClipL out"));

    return iInputCodecsAndResolutions[codec][resolution];
    
    }


// -----------------------------------------------------------------------------
// CVedCodecChecker::IsSupportedOutputFormatL
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//   
TBool CVedCodecChecker::IsSupportedOutputFormatL(const TPtrC8& aMimeType, TSize aResolution)
    {
    
    PRINT(_L("CVedCodecChecker::IsSupportedOutputFormatL in"));
    
    if (aMimeType == KNullDesC8)
    {
        User::Leave(KErrArgument);
    }
    
    if ( !iOutputFormatsChecked )
    {   
        // check supported output formats
        GetSupportedOutputFormatsL();
        iOutputFormatsChecked = ETrue;
    }        
    
    TResolution resolution = MapResolution(aResolution);
    
    if (resolution == EResolutionUnsupported)
        return EFalse;
    
    TCodec codec = ParseMimeType(aMimeType, aResolution);
    
    if (codec == ECodecUnsupported)
        return EFalse;    
    
    PRINT(_L("CVedCodecChecker::IsSupportedOutputFormatL out"));
    
    return iOutputCodecsAndResolutions[codec][resolution];
    
    }
    
    
// -----------------------------------------------------------------------------
// CVedCodecChecker::MapResolution
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//   
TResolution CVedCodecChecker::MapResolution(TSize aResolution)
    {

    if ( aResolution == KVedResolutionSubQCIF )
        return EResolutionSubQCIF;
    
    else if ( aResolution == KVedResolutionQCIF )
        return EResolutionQCIF;
    
    else if ( aResolution == KVedResolutionCIF )
        return EResolutionCIF;
    
    else if ( aResolution == KVedResolutionQVGA )
        return EResolutionQVGA;
    
    else if ( aResolution == KVedResolutionVGA16By9 )
        return EResolutionVGA16By9;
    
    else if ( aResolution == KVedResolutionVGA )
        return EResolutionVGA;
    //WVGA task       
    else if ( aResolution == KVedResolutionWVGA )
        return EResolutionWVGA;
    
    return EResolutionUnsupported;
    }

// -----------------------------------------------------------------------------
// CVedCodecChecker::MapProfileLevelId
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//   
TCodec CVedCodecChecker::MapProfileLevelId(TInt aProfileLevelId)
    {

    if ( aProfileLevelId == 8 )
        return ECodecMPEG4VSPLevel0;
    
    if ( aProfileLevelId == 9 )
        return ECodecMPEG4VSPLevel0B;
    
    if ( aProfileLevelId == 1 )
        return ECodecMPEG4VSPLevel1;
    
    if ( aProfileLevelId == 2 )
        return ECodecMPEG4VSPLevel3;
    
    if ( aProfileLevelId == 3 )
        return ECodecMPEG4VSPLevel3;
    
    if ( aProfileLevelId == 4 )
        return ECodecMPEG4VSPLevel4;
    
    return ECodecUnsupported;

    }
    
// -----------------------------------------------------------------------------
// CVedCodecChecker::MapAVCLevel
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//   
TCodec CVedCodecChecker::MapAVCLevel(TInt aLevel)
    {
    
    if ( aLevel == 10 )
        return ECodecAVCBPLevel1;
    
    if ( aLevel == 101 )
        return ECodecAVCBPLevel1B;
    
    if ( aLevel == 11 )
        return ECodecAVCBPLevel1_1;
    
    if ( aLevel == 12 )
        return ECodecAVCBPLevel1_2;
    
    if ( aLevel == 13 )
        return ECodecAVCBPLevel1_3;
    
    if ( aLevel == 20 )
        return ECodecAVCBPLevel2;    
    //WVGA task
    if ( aLevel == 21 )
        return ECodecAVCBPLevel2_1;
    
    if ( aLevel == 22 )
        return ECodecAVCBPLevel2_2;
    
    if ( aLevel == 30 )
        return ECodecAVCBPLevel3;
    
    if ( aLevel == 31 )
        return ECodecAVCBPLevel3_1; 
    
    return ECodecUnsupported;
    
    }
    
// -----------------------------------------------------------------------------
// CVedCodecChecker::ParseMimeType
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//   
TCodec CVedCodecChecker::ParseMimeType(const TPtrC8& aMimeType, TSize aResolution)
    {

    // figure out codec
    TCodec codec;
    
    if ( aMimeType.MatchF( _L8("*video/H263-2000*") ) != KErrNotFound )
    {
        codec = ECodecH263BPLevel10;
        
        if ( aMimeType.MatchF( _L8("*profile*") ) != KErrNotFound )
        {
            // Profile info is given, check it
            if ( aMimeType.MatchF( _L8("*profile=0*") ) == KErrNotFound )
            {
                return ECodecUnsupported;
            }
        }
        else 
        {
            // no profile given
        }
        
        if ( aMimeType.MatchF( _L8("*level=10*") ) != KErrNotFound )
        {
            codec = ECodecH263BPLevel10;            
        }
            
        else if ( aMimeType.MatchF( _L8("*level=45*") ) != KErrNotFound )
        {            
            codec = ECodecH263BPLevel45;                		
        }
        
        else if ( aMimeType.MatchF( _L8("*level*") ) != KErrNotFound )
        {
            // no other levels supported
            return ECodecUnsupported;
        }
        else
        {
            // if no level is given assume 10
        }        
    }
    else if ( (aMimeType.MatchF( _L8("*video/mp4v-es*") ) != KErrNotFound) || 
              (aMimeType.MatchF( _L8("*video/MP4V-ES*") ) != KErrNotFound) )
    {
    
        codec = ECodecMPEG4VSPLevel0;
        
        // Check profile-level
        if ( aMimeType.MatchF( _L8("*profile-level-id=*") ) != KErrNotFound )
        {
            if ( aMimeType.MatchF( _L8("*profile-level-id=8*") ) != KErrNotFound )
            {
                codec = ECodecMPEG4VSPLevel0;
            }
            else if( aMimeType.MatchF( _L8("*profile-level-id=1*") ) != KErrNotFound )
            {
                codec = ECodecMPEG4VSPLevel1;
            }
            else if ( aMimeType.MatchF( _L8("*profile-level-id=2*") ) != KErrNotFound )
            {
                codec = ECodecMPEG4VSPLevel2;                
            }
            else if ( aMimeType.MatchF( _L8("*profile-level-id=3*") ) != KErrNotFound )
            {
                codec = ECodecMPEG4VSPLevel3;            
            }
            else if ( aMimeType.MatchF( _L8("*profile-level-id=9*") ) != KErrNotFound )
            {
                codec = ECodecMPEG4VSPLevel0B;                
            }
            else if ( aMimeType.MatchF( _L8("*profile-level-id=4*") ) != KErrNotFound )
            {
                codec = ECodecMPEG4VSPLevel4;
            }
            else
            {
                return ECodecUnsupported;                
            }
        
        } 
        else
        {   // no profile-level id
            switch( aResolution.iWidth )
            {                
                case KSubQCIFWidth:
                case KQCIFWidth:
                {
                    // Set profile-level-id=0
                    codec = ECodecMPEG4VSPLevel0;
                    break;
                }
                
                case KCIFWidth:
                case KQVGAWidth:
                {                
                    // Set profile-level-id=2
                    codec = ECodecMPEG4VSPLevel2;
                    break;
                }

                case KVGAWidth:
                {
                    // Set profile-level-id=4 (4a)
                    codec = ECodecMPEG4VSPLevel4;                    
                    break;
                }

                default:
                {
                    // Set profile-level-id=0
                    codec = ECodecMPEG4VSPLevel0;
                }                
            }
            
        }
    } 
    
#ifdef VIDEOEDITORENGINE_AVC_EDITING
    else if ( (aMimeType.MatchF( _L8("*video/h264*") ) != KErrNotFound) || 
              (aMimeType.MatchF( _L8("*video/H264*") ) != KErrNotFound) )
    
    {
        codec = ECodecAVCBPLevel1;
        
        if ( aMimeType.MatchF( _L8("*profile-level-id=42800A*") ) != KErrNotFound )
        {
            codec = ECodecAVCBPLevel1;
        }
        else if( aMimeType.MatchF( _L8("*profile-level-id=42900B*") ) != KErrNotFound )
        {
            codec = ECodecAVCBPLevel1B;
        }
        else if ( aMimeType.MatchF( _L8("*profile-level-id=42800B*") ) != KErrNotFound )
        {
            codec = ECodecAVCBPLevel1_1;                
        }
        else if ( aMimeType.MatchF( _L8("*profile-level-id=42800C*") ) != KErrNotFound )
        {
            codec = ECodecAVCBPLevel1_2;
        }    
        //WVGA task
        else if ( aMimeType.MatchF( _L8("*profile-level-id=42800D*") ) != KErrNotFound )
        {
            codec = ECodecAVCBPLevel1_3;
        } 
        else if ( aMimeType.MatchF( _L8("*profile-level-id=428014*") ) != KErrNotFound )
        {
            codec = ECodecAVCBPLevel2;
        } 
        else if ( aMimeType.MatchF( _L8("*profile-level-id=428015*") ) != KErrNotFound )
        {
            codec = ECodecAVCBPLevel2_1;
        } 
        else if ( aMimeType.MatchF( _L8("*profile-level-id=428016*") ) != KErrNotFound )
        {
            codec = ECodecAVCBPLevel2_2;
        } 
        else if ( aMimeType.MatchF( _L8("*profile-level-id=42801E*") ) != KErrNotFound )
        {
            codec = ECodecAVCBPLevel3;
        } 
        else if ( aMimeType.MatchF( _L8("*profile-level-id=42801F*") ) != KErrNotFound )
        {
            codec = ECodecAVCBPLevel3_1;
        } 
        else
        {
            return ECodecUnsupported;                
        }        
    }
#endif

    else 
    {
        return ECodecUnsupported;
    }
    
    return codec;

}
            
            
// End of file
        
