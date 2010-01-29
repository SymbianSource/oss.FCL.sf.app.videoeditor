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


#ifndef AACCONSTANTS_H
#define AACCONSTANTS_H


// length of a silent mono AAC frame
const TInt KSilentMonoAACFrameLenght = 6;

// silent mono AAC frame
const TUint8 KSilentMonoAACFrame[KSilentMonoAACFrameLenght] =
    {
    0,
    0,
    0,
    128,
    35,
    222
    };

// length of a silent stereo AAC frame
const TInt KSilentStereoAACFrameLenght = 8;

// silent stereo AAC frame
const TUint8 KSilentStereoAACFrame[KSilentStereoAACFrameLenght] =
    {
    33,
    0,
    64,
    0,
    4,
    0,
    0,
    71
    };


// a class to encapsule info needed by AAC frame handler
class TAACFrameHandlerInfo
    {

public:

    TUint8    iNumChannels; 
    TUint8    iNumCouplingChannels;
    TUint8    iSampleRateID;
    TUint8    iProfileID;
    TUint8    iIs960;
    TUint8  isSBR;
    TUint8    iIsParametricStereo;
    };


// AAC specific constants
enum
{
  SCE_ELEMENT = 0,
  CPE_ELEMENT = 1,
  LFE_ELEMENT = 3,

  LC_OBJECT   = 1,
 
  LTP_OBJECT  = 3,



  AAC_ADIF    = 0,


  AAC_ADTS    = 1,

  AAC_MP4     = 2,

  TNS_TOOL    = 1,
 
  LTP_TOOL    = 2,

  PNS_TOOL    = 4,

  MS_TOOL     = 8,
  IS_TOOL     = 16,

  SBR_TOOL    = 32,

  LAST_TOOL   = 32768
};

#endif
