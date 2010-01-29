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


#ifdef __cplusplus
extern "C" {
#endif

#ifndef _AVCDECODER_H_
#define _AVCDECODER_H_

//#include <e32std.h>
#include <e32def.h>


#define AVCD_ERROR                       -1
#define AVCD_OK                          0
#define AVCD_OK_STREAM_NOT_DECODED       1
#define AVCD_OK_FRAME_NOT_AVAILABLE      2

#define AVCD_PICTURE_TYPE_P              0
#define AVCD_PICTURE_TYPE_I              1

#define AVCD_PROFILE_BASELINE            66
#define AVCD_PROFILE_MAIN                77
#define AVCD_PROFILE_EXTENDED            88
#define AVCD_PROFILE_HIGH                100
#define AVCD_PROFILE_HIGH_10             110
#define AVCD_PROFILE_HIGH_422            122
#define AVCD_PROFILE_HIGH_444            144


typedef void avcdDecoder_t;

avcdDecoder_t *avcdOpen();

void avcdClose(avcdDecoder_t *dec);

TInt avcdParseLevel(avcdDecoder_t *dec, void *nalUnitBits, TUint* nalUnitLen, TInt& aLevel);

TInt avcdParseResolution(avcdDecoder_t *dec, void *nalUnitBits, TUint* nalUnitLen, 
                         TInt& aWidth, TInt& aHeight);

#ifdef VIDEOEDITORENGINE_AVC_EDITING

TInt avcdParseParameterSet(avcdDecoder_t *dec, void *nalUnitBits, TUint* nalUnitLen);
                    
TInt avcdParseOneNal(avcdDecoder_t *dec, void *nalUnitBits, TUint* nalUnitLen);

                    
void FrameIsFromEncoder(avcdDecoder_t *dec, TUint aFromEncoder);



TUint8* ReturnPPSSet(avcdDecoder_t *dec, TUint aIndex, TUint* aPPSLength);

TUint ReturnNumPPS(avcdDecoder_t *dec);

TUint8* ReturnSPSSet(avcdDecoder_t *dec, TUint aIndex, TUint* aSPSLength);

TUint ReturnNumSPS(avcdDecoder_t *dec);

TBool ReturnEncodeUntilIDR(avcdDecoder_t *dec);

TInt avcdGenerateNotCodedFrame(avcdDecoder_t *dec, void *aNalUnitBits, TUint aNalUnitLen, TUint aFrameNumber);

TInt avcdStoreCurrentPPSId(avcdDecoder_t *dec, TUint8 *nalUnitBits, TUint aNalUnitLen);

void avcdModifyFrameNumber(avcdDecoder_t *dec, void *aNalUnitBits, TUint aNalUnitLen, TUint aFrameNumber);

#endif // VIDEOEDITORENGINE_AVC_EDITING

#endif


#ifdef __cplusplus
}
#endif
