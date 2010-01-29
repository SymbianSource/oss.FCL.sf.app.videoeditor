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
* A header file for implementation class of AVC editing API.
*               
*/



#ifndef VEDAVCEDITIMP_H
#define VEDAVCEDITIMP_H

// INCLUDES
#include "avcdapi.h"
#include "vedavcedit.h"

// FORWARD DECLARATIONS

// CLASS DECLARATION

/**
*  CTranscoder Implementation class
*  @lib VEDAVCEDIT.LIB
*  @since 
*/

NONSHARABLE_CLASS(CVedAVCEditImp) : public CVedAVCEdit
{

    public:  // Constructors and destructor
    
        /**
        * Two-phased constructor.
        */
        static CVedAVCEditImp* NewL();

        /**
        * Destructor.
        */        
        ~CVedAVCEditImp();
      
    public: // Functions from CVedAVCEdit                    
    
    	/**
        * From CVedAVCEdit Add MDF specific NAL headers to input AVC frame        
        */                
	    void ProcessAVCBitStreamL(TDes8& aBuf, TInt& aFrameLen, TInt aDecInfoSize, TBool aFirstFrame);
	
	    /**	    
 	    * From CVedAVCEdit Calculate default value for max no. of buffered frames
        */        
	    TInt GetMaxAVCFrameBuffering(TInt aLevel, TSize aResolution);
	    
	    /**
        * From CVedAVCEdit Get input bitstream level from SPS
        */
        TInt GetLevel(TDesC8& aBuf, TInt& aLevel);
                
        /**
        * From CVedAVCEdit Get input bitstream resolution from SPS
        */
        TInt GetResolution(TDesC8& aBuf, TSize& aResolution);
	    
#ifdef VIDEOEDITORENGINE_AVC_EDITING
    
    	/**
		* From CVedAVCEdit Convert AVC decoder config info to AVCDecoderConfigurationRecord -format
		*/        
	    void ConvertAVCHeaderL(TDesC8& aSrcBuf, TDes8& aDstBuf );
	    	    
	    /**	    
 	    * From CVedAVCEdit Save SPS/PPS NAL units from AVCDecoderConfigurationRecord
        */        
        void SaveAVCDecoderConfigurationRecordL(TDes8& aBuf, TBool aFromEncoder);              
        
        /**	    
 	    * From CVedAVCEdit Update slice header information: frame number and PPS id        
        */      
        TInt ParseFrame(HBufC8*& aBuf, TBool aContainsDCR, TBool aFromEncoder);
	    
	    /**	    
 	    * From CVedAVCEdit Constructs AVCDecoderConfigurationRecord for the output sequence
        */      
        void ConstructAVCDecoderConfigurationRecordL( TDes8& aDstBuf );
        
        /**	    
 	    * From CVedAVCEdit Set length of NAL size field of output sequence
        */      
        void inline SetNALLengthSize( TInt aSize ) { iNalLengthSize = aSize; }
        
         /**	    
 	    * From CVedAVCEdit Set output level for constructing AVCDecoderConfigurationRecord
        */     
        void inline SetOutputLevel(TInt aLevel) { iOutputLevel = aLevel; }
        
        /**	    
 	    * From CVedAVCEdit Check if encoding is required until the next IDR unit
        */     
        TBool EncodeUntilIDR();

        /**	    
 	    * From CVedAVCEdit Check if NAL unit is an IDR NAL
        */     
        TBool IsNALUnitIDR( TDes8& aNalBuf );

        /**	    
 	    * From CVedAVCEdit Store PPS id from current slice
        */     
		void StoreCurrentPPSId( TDes8& aNalBuf );
        
        /**	    
 	    * From CVedAVCEdit Generate a not coded frame
        */     
		TInt GenerateNotCodedFrame( TDes8& aNalBuf, TUint aFrameNumber );
		
        /**
        * From CVedAVCEdit Modify the frame number of the NAL unit
        */	   
		void ModifyFrameNumber( TDes8& aNalBuf, TUint aFrameNumber );
		
private:  // Private methods
        
        /*
        * Saves one SPS/PPS NAL unit for later use
        *  
        * @param aBuf Input buffer containing AVCDecoderConfigurationRecord        
        */                	    
	    TInt ParseOneNAL(void *aNalUnitData, TUint* aNalUnitLength, TBool aFromEncoder);
	    
	    TInt ParseParameterSet(void *aNalUnitData, TUint* aNalUnitLength, TBool aFromEncoder);
#endif
        
    private:
    
        /**
        * C++ default constructor.
        */
        CVedAVCEditImp();
        
        /**
        * 2nd phase constructor 
        */
        void ConstructL();
	    
	private:  // Data
	
	    avcdDecoder_t* iAvcDecoder;  // For parsing AVC content / for CD operations 
	
	    TInt iFrameLengthBytes;  // no. of bytes used for length field	    	    						
	    
#ifdef VIDEOEDITORENGINE_AVC_EDITING
				
	    TInt iNalLengthSize;	// The number of bytes used to signal NAL unit's length 
	    
	    TInt iOutputLevel;  // output level
	    
#endif

};


#endif

// End of File
