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
#include "VeiImageConverter.h"

#include <fbs.h>
#include <ImageConversion.h>
#include <BitmapTransforms.h>
#include "VideoEditorCommon.h" 
#include "VideoEditorDebugUtils.h"



// ================= MEMBER FUNCTIONS =======================

// ---------------------------------------------------------
// CVeiImageConverter::CVeiImageConverter::NewL(
//		MConverterController* aController ) 
// ---------------------------------------------------------
//
EXPORT_C CVeiImageConverter* CVeiImageConverter::NewL( 
	MConverterController* aController )
	{
	CVeiImageConverter* self = 
		new(ELeave) CVeiImageConverter( aController );
	CleanupStack::PushL( self );
	
	self->ConstructL();

	CleanupStack::Pop( self );
	return self; 
	}
// ---------------------------------------------------------
// CVeiImageConverter::CVeiImageConverter(
//		MConverterController* aController )
// ---------------------------------------------------------
//
CVeiImageConverter::CVeiImageConverter( 
	MConverterController* aController ) : 
	CActive( EPriorityStandard ), iController( aController )
	{
	}

// ---------------------------------------------------------
// CVeiImageConverter::ConstructL()
// EPOC two phased constructor
// ---------------------------------------------------------
//
void CVeiImageConverter::ConstructL()
	{
	User::LeaveIfError( iFs.Connect() );
	// create the destination bitmap
	
	iBitmapScaler = CBitmapScaler::NewL();
	
	CActiveScheduler::Add( this );
	}

// Destructor
EXPORT_C CVeiImageConverter::~CVeiImageConverter()
	{
	LOG(KVideoEditorLogFile, "CVeiImageConverter::~CVeiImageConverter(): in");
	if ( IsActive() )
		{
		Cancel();	
		}

	delete iImageEncoder; 
	delete iBitmapScaler;
							// CImageDecoder must be deleted before the 
	iFs.Close();			//   related RFs is closed, 
	delete iBitmap;			//   otherwise a related thread might panic	

	iController = NULL;

	LOG(KVideoEditorLogFile, "CVeiImageConverter::~CVeiImageConverter(): out");
	}	



// ---------------------------------------------------------
// CVeiImageConverter::StartToEncodeL( 
//		TFileName& aFileName, TUid aImageType, TUid aImageSubType )
// ---------------------------------------------------------
//
EXPORT_C void CVeiImageConverter::StartToEncodeL( 
	const TDesC& aFileName, const TUid& aImageType, const TUid& aImageSubType )
	{
	LOG(KVideoEditorLogFile, "CVeiImageConverter::StartToEncodeL(): In");
	if ( iImageEncoder )
		{
		delete iImageEncoder; 
		iImageEncoder = NULL;
		}
	
	// create the encoder
	LOG(KVideoEditorLogFile, "CVeiImageConverter::StartToEncodeL(): 2, own thread started");
	iImageEncoder = CImageEncoder::FileNewL( iFs, aFileName, 
		CImageEncoder::EOptionAlwaysThread, aImageType, aImageSubType );	

	iState = EEncoding;
	iImageEncoder->Convert( &iStatus, *iBitmap );
	
	SetActive();
	LOG(KVideoEditorLogFile, "CVeiImageConverter::StartToEncodeL(): Out");
	}
// ---------------------------------------------------------
// CVeiImageConverter::DoCancel()
// ---------------------------------------------------------
//
void CVeiImageConverter::DoCancel()
	{
	LOG(KVideoEditorLogFile, "CVeiImageConverter::DoCancel(): in");
	if ( iState == EEncoding && iImageEncoder )
		{
		iImageEncoder->Cancel();
		}
	else if ( iState == EScaling )
		{
		iBitmapScaler->Cancel();
		}
	LOG(KVideoEditorLogFile, "CVeiImageConverter::DoCancel(): out");
	}
// ---------------------------------------------------------
// CVeiImageConverter::RunL()
// ---------------------------------------------------------
//
void CVeiImageConverter::RunL()
	{
	LOG(KVideoEditorLogFile, "CVeiImageConverter::RunL: In");
	switch( iState ) 
		{
		case EEncoding:
			{
			iState = EIdle;
			iController->NotifyCompletion( iStatus.Int() );
			break;
			}
		
		default: // some error
			{
			LOG(KVideoEditorLogFile, "CVeiImageConverter::RunL: Scale completed");
			iState = EIdle;
			iController->NotifyCompletion( iStatus.Int() );
			}
		}
	}

// ---------------------------------------------------------
// CVeiImageConverter::RunError(TInt aError)
// ---------------------------------------------------------
//
TInt CVeiImageConverter::RunError(TInt aError)
	{
	LOGFMT(KVideoEditorLogFile, "CVeiImageConverter::RunError(): in, aError:%d", aError);
	TInt err;
	err = aError;
	return err;
	}

// ---------------------------------------------------------
// CVeiImageConverter::GetEncoderImageTypesL(
//		RImageTypeDescriptionArray& aImageTypeArray )
// ---------------------------------------------------------
//
EXPORT_C void CVeiImageConverter::GetEncoderImageTypesL( 
	RImageTypeDescriptionArray& aImageTypeArray )
	{
	CImageEncoder::GetImageTypesL( aImageTypeArray );
	}

// ---------------------------------------------------------
// CVeiImageConverter::SetBitmap()
// ---------------------------------------------------------
//
EXPORT_C void CVeiImageConverter::SetBitmap( CFbsBitmap* aBitmap )
	{
	LOG(KVideoEditorLogFile, "CVeiImageConverter::SetBitmap(): in");
	if (iBitmap)
		{
		delete iBitmap;
		iBitmap = NULL;
		}
	iBitmap = aBitmap;
	LOG(KVideoEditorLogFile, "CVeiImageConverter::SetBitmap(): out");
	}

// ---------------------------------------------------------
// CVeiImageConverter::GetBitmap()
// ---------------------------------------------------------
//
EXPORT_C CFbsBitmap* CVeiImageConverter::GetBitmap()
	{
	return iBitmap;
	}

// ---------------------------------------------------------
// CVeiImageConverter::Scale()
// ---------------------------------------------------------
//
EXPORT_C void CVeiImageConverter::ScaleL(CFbsBitmap* aSrcBitmap,CFbsBitmap* aDestBitmap, const TSize& aSize)
	{
	LOG(KVideoEditorLogFile, "CVeiImageConverter::Scale: in");
	if ( IsActive() )
		{
		Cancel();
		}

	if ( iBitmap )
		{
		delete iBitmap;
		iBitmap = NULL;
		}
/* Create new bitmap where scaled bitmap is stored
	When scaling is ready, NotifyCompletion() callback and
	bitmap address can be asked with GetBitmap function */

	iBitmap = aDestBitmap;
	iBitmap = new (ELeave) CFbsBitmap;
	iBitmap->Create( aSize, aSrcBitmap->DisplayMode() );

	iState = EScaling;
	iBitmapScaler->Scale( &iStatus, *aSrcBitmap, *iBitmap, EFalse );

	SetActive();

	LOG(KVideoEditorLogFile, "CVeiImageConverter::Scale: out");
	}

// ---------------------------------------------------------
// CVeiImageConverter::CancelEncoding()
// ---------------------------------------------------------
//
EXPORT_C void CVeiImageConverter::CancelEncoding()
	{
	if ( iImageEncoder )
		{
		delete iImageEncoder; 
		iImageEncoder = NULL;
		} 
	}

// End of File
