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



// INCLUDE FILES
#include <e32svr.h>
#include <fbs.h>
#include "VedRgb2YuvConverter.h"

// EXTERNAL DATA STRUCTURES

// EXTERNAL FUNCTION PROTOTYPES  

// CONSTANTS

// MACROS
#ifdef _DEBUG
#	define __IF_DEBUG(t) {RDebug::t;}
#else
#	define __IF_DEBUG(t)
#endif

// LOCAL CONSTANTS AND MACROS

// ?one_line_short_description_of_data
#define AVG(a,b) ( ( a + b ) >> 1 )

// MODULE DATA STRUCTURES

/**
*  ?one_line_short_description.
*  ?other_description_lines
*
*  @lib ?library
*  @since ?Series60_version
*/
struct TVSYCrCb
	{
	public:
		// ?one_line_short_description_of_data
		TInt iY;
		
		// ?one_line_short_description_of_data
		TInt iCb;
		
		// ?one_line_short_description_of_data
		TInt iCr;
	};

// LOCAL FUNCTION PROTOTYPES

// FORWARD DECLARATIONS

// ============================= LOCAL FUNCTIONS ===============================

// -----------------------------------------------------------------------------
// ?classname::?member_function
// ?implementation_description
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
inline TUint8 RGBtoYCbCr( TVSYCrCb* aYuv, const TRgb& aColor )
	{
	const TInt YRedFactor = 19595; // 0.299 << 16
	const TInt YGreenFactor = 38470; // 0.587 << 16
	const TInt YBlueFactor = 7471; // 0.114 << 16
	const TInt CbRedFactor = 11056; // 0.1687 << 16
	const TInt CbGreenFactor = 21712; // 0.3313 << 16
	const TInt CrGreenFactor = 27440; // 0.4187 << 16
	const TInt CrBlueFactor = 5328; // 0.0813 << 16

	TInt red = aColor.Red();
	TInt green = aColor.Green();
	TInt blue = aColor.Blue();

	aYuv->iY = (YRedFactor * red) + (YGreenFactor * green) + (YBlueFactor * blue);
	aYuv->iCb = - (CbRedFactor * red) - (CbGreenFactor * green) + (blue << 15);
	aYuv->iCr = (red << 15) - (CrGreenFactor * green) - (CrBlueFactor * blue);

	aYuv->iY >>= 16;
	aYuv->iCb >>= 16;
	aYuv->iCr >>= 16;

	aYuv->iCb += 128;
	aYuv->iCr += 128;

	return static_cast<TUint8>( aYuv->iY );
	}

// -----------------------------------------------------------------------------
// ?classname::?member_function
// ?implementation_description
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
TRgb VSReadColor4K( TAny*& aSource )
	{
	TUint16* s = static_cast<TUint16*>( aSource );
	TRgb rgb( TRgb::Color4K( *s++ ) );
	aSource = s;
	return rgb;
	}

// -----------------------------------------------------------------------------
// ?classname::?member_function
// ?implementation_description
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
TRgb VSReadColor64K( TAny*& aSource )
	{
	TUint16* s = static_cast<TUint16*>( aSource );
	TRgb rgb( TRgb::Color64K( *s++ ) );
	aSource = s;
	return rgb;
	}

// -----------------------------------------------------------------------------
// ?classname::?member_function
// ?implementation_description
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
TRgb VSReadColor16M( TAny*& aSource )
	{
	TUint8* s = static_cast<TUint8*>( aSource );
	TRgb rgb( s[2], s[1], s[0] );
	aSource = s + 3;
	return rgb;
	}

// ============================ MEMBER FUNCTIONS ===============================

// ============================ CVSFbsBitmapYUV420Converter ===============================

// -----------------------------------------------------------------------------
// ?classname::?member_function
// ?implementation_description
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
CVSFbsBitmapYUV420Converter* CVSFbsBitmapYUV420Converter::NewL( const CFbsBitmap& aBitmap )
	{
	CVSFbsBitmapYUV420Converter* self = new (ELeave) CVSFbsBitmapYUV420Converter();
	CleanupStack::PushL( self );
	self->ConstructL( aBitmap );
	CleanupStack::Pop(); // self
	return self;
	}

// -----------------------------------------------------------------------------
// ?classname::?member_function
// ?implementation_description
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
CVSFbsBitmapYUV420Converter::~CVSFbsBitmapYUV420Converter()
	{
	delete iSource;
	delete iYUVData;
	}

// -----------------------------------------------------------------------------
// ?classname::?member_function
// ?implementation_description
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CVSFbsBitmapYUV420Converter::SetSourceL( const CFbsBitmap& aBitmap )
	{
	ReConstructL( aBitmap );
	}

// -----------------------------------------------------------------------------
// ?classname::?member_function
// ?implementation_description
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CVSFbsBitmapYUV420Converter::ProcessL()
	{
	switch( iSource->DisplayMode() )
		{
		case EColor4K:
			DoProcess( VSReadColor4K );
			break;

		case EColor64K:
			DoProcess( VSReadColor64K );
			break;
		
		case EColor16M:
			DoProcess( VSReadColor16M );
			break;

		default:
			User::Leave( KErrNotSupported );
			break;
		};
	}

// -----------------------------------------------------------------------------
// ?classname::?member_function
// ?implementation_description
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
TPtrC8 CVSFbsBitmapYUV420Converter::YUVData() const
	{
	return *iYUVData;
	}

// -----------------------------------------------------------------------------
// ?classname::?member_function
// ?implementation_description
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CVSFbsBitmapYUV420Converter::ConstructL( const CFbsBitmap& aBitmap )
	{
	iSource = new (ELeave) CFbsBitmap();
	ReConstructL( aBitmap );
	}

// -----------------------------------------------------------------------------
// ?classname::?member_function
// ?implementation_description
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CVSFbsBitmapYUV420Converter::ReConstructL( const CFbsBitmap& aBitmap )
	{
	User::LeaveIfError( iSource->Duplicate( aBitmap.Handle() ) );
	
	// make sure that destination bitmap's displaymode is supported
	if( ( iSource->DisplayMode() != EColor4K ) && ( iSource->DisplayMode() != EColor64K ) && ( iSource->DisplayMode() != EColor16M ) )
		{
		User::Leave( KErrNotSupported );
		}
	
	if( !iYUVData || !( iSize == iSource->SizeInPixels() ) )
		{
		iSize = iSource->SizeInPixels();
		delete iYUVData; iYUVData = 0;
		TInt ySize = iSize.iWidth * iSize.iHeight;
		iYUVData = HBufC8::NewMaxL( ySize + ( ySize >> 1 ) );
		iY.Set( iYUVData->Des().Mid( 0, ySize ) );
		iU.Set( iYUVData->Des().Mid( ySize, ySize >> 2 ) );
		iV.Set( iYUVData->Des().Mid( ySize + ( ySize >> 2 ), ySize >> 2 ) );
		}
	}

// -----------------------------------------------------------------------------
// ?classname::?member_function
// ?implementation_description
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CVSFbsBitmapYUV420Converter::DoProcess( TVSColorReadFunc aReadFunction )
	{
	TUint8* pY = const_cast<TUint8*>( iY.Ptr() );
	TUint8* pU = const_cast<TUint8*>( iU.Ptr() );
	TUint8* pV = const_cast<TUint8*>( iV.Ptr() );
	TVSYCrCb yuv1, yuv2;

	iSource->LockHeap();
	TAny* s = iSource->DataAddress();
	for( TInt h = 0; h < iSize.iHeight; h++ )
		{
		for( TInt w = 0; w < iSize.iWidth>>1; w++ ) // Note! width must be even divisible by 2
			{
			*pY++ = RGBtoYCbCr( &yuv1, aReadFunction( s ) ); 
			*pY++ = RGBtoYCbCr( &yuv2, aReadFunction( s ) ); 
			if( h&1 )
				{
				*pU++ = static_cast<TUint8>( AVG( yuv1.iCb, yuv2.iCb ) );
				*pV++ = static_cast<TUint8>( AVG( yuv1.iCr, yuv2.iCr ) );
				}
			else
				{
				*pU++ = static_cast<TUint8>( AVG( *pU, AVG( yuv1.iCb, yuv2.iCb ) ) );
				*pV++ = static_cast<TUint8>( AVG( *pV, AVG( yuv1.iCr, yuv2.iCr ) ) );
				}
			}
		// if even row -> decrease pU and pV, we will calculate average for those on odd rows
		if( !(h&1) )
			{
			pU -= ( iSize.iWidth >> 1 );
			pV -= ( iSize.iWidth >> 1 );
			}
		}
	iSource->UnlockHeap();
	}

//  End of File
