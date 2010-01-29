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
* A header file defining the AVC editing API.
*                
*
*/



#ifndef VEDAVCEDIT_H
#define VEDAVCEDIT_H

#include <e32base.h>

/**
*  CVedAVCEdit abstract base class for AVC editing class
*
*  @lib vedAVCEdit
*  @since
*/
class CVedAVCEdit : public CBase
    {

    public:  // Constants

    public:  // Constructors and destructor        	

        /**
        * Two-phased constructor.
        *
        */
        IMPORT_C static CVedAVCEdit* NewL();
        
        /**
        * Destructor.
        */
        virtual ~CVedAVCEdit();

    public:  // New functions
    
    	/*
        * Add MDF specific NAL headers to input AVC frame
        * 
        * @param aBuf Input/output buffer
        * @param aFrameLen Input/output frame length
        * @param aDecInfoSize Decoder specific info size
        * @param aFirstFrame ETrue for first frame in sequence
        */                
	    virtual void ProcessAVCBitStreamL(TDes8& aBuf, TInt& aFrameLen, TInt aDecInfoSize, TBool aFirstFrame) = 0;		

 	    /*
        * Calculate default value for no. of buffered frames according to 
        * H.264 spec Annex A
        * 
        * @param aLevel AVC level
        * @param aResolution Video resolution        
        */
	    virtual TInt GetMaxAVCFrameBuffering(TInt aLevel, TSize aResolution) = 0;
	    
	    /*
        * Get input bitstream level from SPS. Note: SPS Nal unit must be at the 
        * beginning of the input buffer
        * 
        * @param aBuf Input buffer containing SPS
        * @param aLevel Output: Baseline profile level
        */        
        virtual TInt GetLevel(TDesC8& aBuf, TInt& aLevel) = 0;
                
        /*
        * Get input bitstream video resolution from SPS. Note: SPS Nal unit must 
        * be at the beginning of the input buffer
        * 
        * @param aBuf        Input buffer containing SPS
        * @param aResolution Output: Resolution
        */        
        virtual TInt GetResolution(TDesC8& aBuf, TSize& aResolution) = 0;
     
#ifdef VIDEOEDITORENGINE_AVC_EDITING

        /*
        * Convert AVC specific decoder config info to AVCDecoderConfigurationRecord -format
        * 
        * @param aSrcBuf Source buffer containing SPS/PPS NAL units in AVC MDF buffer format
        * @param aDstBuf Destination buffer for AVCDecoderConfigurationRecord        
        */        
        virtual void ConvertAVCHeaderL(TDesC8& aSrcBuf, TDes8& aDstBuf ) = 0;	    

	    /*
        * Save SPS/PPS NAL units from AVCDecoderConfigurationRecord
        * for later use
        *  
        * @param aBuf Input buffer containing AVCDecoderConfigurationRecord        
        */                
	    virtual void SaveAVCDecoderConfigurationRecordL(TDes8& aBuf, TBool aFromEncoder) = 0;	    
	    
	    /*
        * Update slice header information: frame number and PPS id        
        * 
        * @param aBuf Buffer containing input frame
        * @param aContainsDCR ETrue if AVC decoder config. record is included
        * @param aFromEncoder ETrue if frame comes from the encoder
        */	    
        virtual TInt ParseFrame(HBufC8*& aBuf, TBool aContainsDCR, TBool aFromEncoder) = 0;
        
        /*
        * Constructs AVCDecoderConfigurationRecord for the whole sequence, taking
        * into account all input DCR's        
        *  
        * @param aDstBuf Destination buffer for AVCDecoderConfigurationRecord        
        * @param aLevel Level output sequence    
        */      
        virtual void ConstructAVCDecoderConfigurationRecordL( TDes8& aDstBuf ) = 0;

        /*
        * Set length of NAL size field of output sequence
        * 
        * @param aSize Length of size field in bytes        
        */	    
        virtual void SetNALLengthSize( TInt aSize ) = 0;
        
        /*
        * Set output level for constructing AVCDecoderConfigurationRecord
        * 
        * @param aLevel Output level
        */	   
        virtual void SetOutputLevel(TInt aLevel) = 0;
        
        /*
        * Check if encoding is required until the next IDR unit
        * 
        */	   
        virtual TBool EncodeUntilIDR() = 0;
        
        /*
        * Check if NAL unit is an IDR NAL
        * 
        * @param aNalBuf buffer containing NAL unit
        */	   
        virtual TBool IsNALUnitIDR( TDes8& aNalBuf ) = 0;
        
        /*
        * Store PPS id from the current slice
        * 
        * @param aNalBuf buffer containing NAL unit
        */	   
		virtual void StoreCurrentPPSId( TDes8& aNalBuf ) = 0;

        /*
        * Generate a not coded frame
        * 
        * @param aNalBuf buffer containing NAL unit
        */	   
		virtual TInt GenerateNotCodedFrame( TDes8& aNalBuf, TUint aFrameNumber ) = 0;

        /*
        * Modify the frame number of the NAL unit
        * 
        * @param aNalBuf buffer containing NAL unit
        */	   
		virtual void ModifyFrameNumber( TDes8& aNalBuf, TUint aFrameNumber ) = 0;
#endif

};


#endif

// End of File
