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
#ifndef IMAGECONVERTER_H
#define IMAGECONVERTER_H

#include <e32std.h>
#include <e32base.h>
#include <f32file.h>
#include <ImageConversion.h>

class CFbsBitmap;
class CBitmapRotator;
class CBitmapScaler;

/**
 * Observer for notifying that image conversion is ready.
 * 
 */
class MConverterController
	{
	public:
		/**
		* Called to notify that image conversion is completed
		* 
		*/
		virtual void NotifyCompletion( TInt aErr ) = 0;
		
	};
/**
* Utility class for image conversion.
*/
NONSHARABLE_CLASS( CVeiImageConverter ) : public CActive
	{
	// states for this object
	enum TState 
		{
		EIdle = 0,
		EEncoding,
		EScaling
		};

	public: // contructors/destructors

		/**
		* NewL 
		* Create a CVeiImageConverter object and return a pointer to it.
		*
		* @param aController Pointer to a MConverterController interface.
		* The engine uses NotifyCompletion callback from this interface
		* to notify the controller about completions of coding or 
		* encoding requests.
		*		 
		* @return a pointer to the created engine
		*/	
		IMPORT_C static CVeiImageConverter* NewL( MConverterController* aController );
	
		IMPORT_C ~CVeiImageConverter();

	public: // interface methods

		IMPORT_C CFbsBitmap* GetBitmap();
		IMPORT_C void SetBitmap(CFbsBitmap* aBitmap);

		/** StartToEncodeL 
		* Starts to encode an image to a file. When completed calls 
		* NotifyCompletion, from iController.
		*
		* @param aFileName Full path and filename to the image to be encoded.
		*		 
		* @returns Nothing
		*/
		IMPORT_C void StartToEncodeL( const TDesC& aFileName, 
			const TUid& aImageType, const TUid& aImageSubType );
		
		/**
		* GetEncoderImageTypesL
		* Gets descriptions of supported (encoding) image types. 
		*
		* @param aImageTypeArray Reference to an array to be filled.
		*
		* @return Nothing 
		*/
		IMPORT_C static void GetEncoderImageTypesL( 
			RImageTypeDescriptionArray& aImageTypeArray );

		IMPORT_C void CancelEncoding();
		
		IMPORT_C void ScaleL(CFbsBitmap* aSrcBitmap,CFbsBitmap* aDestBitmap, const TSize& aSize);
	

	protected: // implementation of CActive
		void DoCancel();
		void RunL();
		TInt RunError(TInt aError);

	private: // internal methods
		CVeiImageConverter( MConverterController* aController ); 
		void ConstructL();

	public: // data	

		/** Decoded image. */
		CFbsBitmap* iBitmap;

	private: // internal data

		/** ui controller. */
		MConverterController* iController; 
		
		/** for opening/saving images from/to files. */
		RFs iFs; 
		
		/** decoder from ICL API. */
		//CImageDecoder* iImageDecoder; // 

		/** encoder from ICL API. */
		CImageEncoder* iImageEncoder; 
		CBitmapScaler* iBitmapScaler;
	
		TState iState;
	};

#endif 
