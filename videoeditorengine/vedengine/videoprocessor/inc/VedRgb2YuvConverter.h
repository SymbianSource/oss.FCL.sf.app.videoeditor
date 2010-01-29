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


#ifndef VIDEOEDITORTESTIMAGECONVERTER_H
#define VIDEOEDITORTESTIMAGECONVERTER_H

// INCLUDE FILES
#include <e32base.h>
#include <gdi.h>

// CLASS FORWARDS
class CFbsBitmap;

// TYPEDEFS
typedef TRgb ( *TVSColorReadFunc ) ( TAny*& );

/**
*  ?one_line_short_description.
*  ?other_description_lines
*
*  @lib ?library
*  @since ?Series60_version
*/
class CVSFbsBitmapYUV420Converter : public CBase
	{
	public:
		/**
        * ?member_description.
        * @since ?Series60_version
        * @param ?arg1 ?description
        * @return ?description
        */
		static CVSFbsBitmapYUV420Converter* NewL( const CFbsBitmap& aBitmap );

		/**
        * ?member_description.
        * @since ?Series60_version
        * @param ?arg1 ?description
        * @return ?description
        */
		~CVSFbsBitmapYUV420Converter();

		/**
        * ?member_description.
        * @since ?Series60_version
        * @param ?arg1 ?description
        * @return ?description
        */
		void SetSourceL( const CFbsBitmap& aBitmap );

		/**
        * ?member_description.
        * @since ?Series60_version
        * @param ?arg1 ?description
        * @return ?description
        */
		void ProcessL();

		/**
        * ?member_description.
        * @since ?Series60_version
        * @param ?arg1 ?description
        * @return ?description
        */
		TPtrC8 YUVData() const;

	private: // internal
		/**
        * ?member_description.
        * @since ?Series60_version
        * @param ?arg1 ?description
        * @return ?description
        */
		void ConstructL( const CFbsBitmap& aBitmap );

		/**
        * ?member_description.
        * @since ?Series60_version
        * @param ?arg1 ?description
        * @return ?description
        */
		void ReConstructL( const CFbsBitmap& aBitmap );

		/**
        * ?member_description.
        * @since ?Series60_version
        * @param ?arg1 ?description
        * @return ?description
        */
		void DoProcess( TVSColorReadFunc aReadFunction );

	private:
		// ?one_line_short_description_of_data
		TSize iSize;

		// ?one_line_short_description_of_data
		CFbsBitmap* iSource; // owned, duplicate

		// ?one_line_short_description_of_data
		HBufC8* iYUVData; // owned

		// ?one_line_short_description_of_data
		TPtrC8 iY;

		// ?one_line_short_description_of_data
		TPtrC8 iU;
		
		// ?one_line_short_description_of_data
		TPtrC8 iV;
	};

#endif      // CVTIMAGECONVERTER_H  
            
// End of File
