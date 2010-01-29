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
* Video settings.
*
*/



#ifndef CTRHWSETTINGS_H
#define CTRHWSETTINGS_H

// Constants for processing time estimates
const TReal KTRDecodeTimeFactorH263HW = 0.04;
const TReal KTRDecodeTimeFactorH263SW = 0.05;
const TReal KTRDecodeTimeFactorH264HW = 0.05;
const TReal KTRDecodeTimeFactorH264SW = 0.05;
const TReal KTRDecodeTimeFactorMPEG4HW = 0.05;
const TReal KTRDecodeTimeFactorMPEG4SW = 0.05;

const TReal KTREncodeTimeFactorH263HW = 0.07;
const TReal KTREncodeTimeFactorH263SW = 0.08;
const TReal KTREncodeTimeFactorH264HW = 0.07;
const TReal KTREncodeTimeFactorH264SW = 0.08;
const TReal KTREncodeTimeFactorMPEG4HW = 0.07;
const TReal KTREncodeTimeFactorMPEG4SW = 0.08;

const TReal KTRResampleTimeFactorBilinear = 0.06;
const TReal KTRResampleTimeFactorDouble = 0.05;
const TReal KTRResampleTimeFactorHalve = 0.05;

const TReal KTRTimeFactorScale = 1.0 / 640.0;

const TInt KTRFallbackDecoderUidH263 = 0x10206674;     // ARM Decoder
const TInt KTRFallbackDecoderUidH264 = 0x102073ef;     // ARM Decoder
const TInt KTRFallbackDecoderUidMPEG4 = 0x10206674;    // ARM Decoder

const TInt KTRFallbackEncoderUidH263 = 0x10282CFC;     // ARM Encoder
const TInt KTRFallbackEncoderUidH264 = 0x20001C13;     // ARM Encoder
const TInt KTRFallbackEncoderUidMPEG4 = 0x10282CFD;    // ARM Encoder

const TInt KTRMaxFramesInProcessingDefault = 3;
const TInt KTRMaxFramesInProcessingScaling = 5;

const TReal KTRWideThreshold = 1.5;

#endif // CTRHWSETTINGS_H
