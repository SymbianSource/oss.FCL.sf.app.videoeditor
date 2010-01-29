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
* Generic API for audio codecs.  
*
*/

#ifndef __AUDPARAMS_H__
#define __AUDPARAMS_H__


// INCLUDES

#include <E32Base.h>


// CLASS DEFINITIONS
/*
-----------------------------------------------------------------------------

    TAudioCodecParams

    Audio codec parameters.

-----------------------------------------------------------------------------
*/
class TAudioCodecParams
    {
public:
    enum TParamsType
        {
        ETypeDecoder = 0,
        ETypeEncoder
        };
public:
    virtual TInt CodecId() const = 0;
    virtual TInt Type() const = 0;
    };


/*
-----------------------------------------------------------------------------

    TAudioDecoderParams

    Decoding parameters.

-----------------------------------------------------------------------------
*/
class TAudioDecoderParams : public TAudioCodecParams
    {
public:
    IMPORT_C virtual TInt Type() const;
    };


/*
-----------------------------------------------------------------------------

    TAudioEncoderParams

    Encoding parameters.

-----------------------------------------------------------------------------
*/
class TAudioEncoderParams : public TAudioCodecParams
    {
public:
    IMPORT_C virtual TInt Type() const;
    };


#endif //__AUDPARAMS_H__
//-----------------------------------------------------------------------------
//  End of File
//-----------------------------------------------------------------------------
