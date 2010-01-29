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
* Header file for H.263 Decoder API implementation class.
*
*/


#ifndef VEDH263DIMP_H
#define VEDH263DIMP_H

#include "h263dext.h"
#include "h263dmai.h"  // CVedH263Dec defined here


class CVedH263DecImp : public CVedH263Dec
{

    public:  // Constructors and destructor
    
        /**
        * C++ default constructor.
        */
        CVedH263DecImp(TSize aFrameSize, TInt aNumReferencePictures);

        /**
        * Destructor.
        */        
        ~CVedH263DecImp();

        /**
        * 2nd phase constructor must be public since it is called from NewL of abstract base class.
        */
        void ConstructL();

    public: // Functions from CVedH263Dec                    

        /**
        * From CVedH263Dec Sets the renderer to be used
        */    
        void SetRendererL(MVideoRenderer* aRenderer);
        
        /**
        * From CVedH263Dec Sets the post-filter to be used
        */ 
        void SetPostFilterL(const TPostFilter aFilter);

        /**
        * From CVedH263Dec Returns the frame to the decoder subsystem
        */ 
        void FrameRendered(const TAny *aFrame);               

        /**
        * From CVedH263Dec Checks if the given frame is valid
        */ 
        TBool FrameValid(const TAny *aFrame);

        /**
        * From CVedH263Dec Retrieves Y/U/V pointers to the given frame
        */ 
        TInt GetYUVBuffers(const TAny *aFrame, TUint8*& aYFrame, TUint8*& aUFrame, 
                           TUint8*& aVFrame, TSize& aFrameSize);

        /**
        * From CVedH263Dec Retrieves the latest decoded YUV frame
        */ 
        TUint8* GetYUVFrame();

        /**
        * From CVedH263Dec Decodes / transcodes a compressed frame
        */ 
        void DecodeFrameL(const TPtrC8& aInputBuffer, TPtr8& aOutputBuffer,
                          TBool& aFirstFrame, TInt& aBytesDecoded,
                          vdeDecodeParamters_t *aDecoderInfo);

		void DecodeFrameL(const TPtrC8& aInputBuffer, TPtr8& aOutputBuffer,                                  TBool& aFirstFrame, TInt& aBytesDecoded, 
			              const TColorEffect aColorEffect,
						  const TBool aGetDecodedFrame, TInt aFrameOperation,
						  TInt* aTrP, TInt* aTrD, TInt aVideoClipNumber, TInt aSMSpeed, 
						  TInt aDataFormat);
        
        /**
        * From CVedH263Dec Decodes a compressed frame
        */ 
        void DecodeFrameL(const TPtrC8& aInputBuffer, TBool& aFirstFrame, TInt& aBytesDecoded, 
					TInt aDataFormat);

        
        /**
        * From CVedH263Dec Check the VOS header
        */ 
		TBool CheckVOSHeaderL(TPtrC8& aInputBuffer);


private:

    // H.263 decoder instance handle
    h263dHInstance_t iH263dHandle; 

    // frame dimensions
    TSize iFrameSize;  

};






#endif
