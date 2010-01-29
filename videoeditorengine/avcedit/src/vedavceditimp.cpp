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
* Implementation class for AVC editing operations.
*
*/


#include <e32svr.h>  
#include <mmf/devVideo/avc.h>

#include "biblin.h"
#include "bitbuffer.h"
#include "vld.h"
#include "vedavceditimp.h"

// Debug print macro
#ifdef _DEBUG   
#include <e32svr.h>
#define PRINT(x) RDebug::Print x;
#else
#define PRINT(x)
#endif

// An assertion macro wrapper to clean up the code a bit
#define VPASSERT(x) __ASSERT_DEBUG(x, User::Panic(_L("CVedAVCEdit"), -10000))

// ================= MEMBER FUNCTIONS =======================

// Two-phased constructor
CVedAVCEditImp* CVedAVCEditImp::NewL()
{
    CVedAVCEditImp* self = new (ELeave) CVedAVCEditImp();
    CleanupStack::PushL(self);
    self->ConstructL();
    CleanupStack::Pop();    
    return self;    
}

// C++ default constructor
CVedAVCEditImp::CVedAVCEditImp()
    {    
    }

// Symbian OS default constructor can leave
void CVedAVCEditImp::ConstructL()
    {
    
    iAvcDecoder = avcdOpen();
    
    if (!iAvcDecoder)
        {
        User::Leave(KErrNoMemory);
        }

#ifdef VIDEOEDITORENGINE_AVC_EDITING
	iNalLengthSize = 4;
	iOutputLevel = 10;
#endif
    
    }

// Destructor
CVedAVCEditImp::~CVedAVCEditImp()
    {
    avcdClose(iAvcDecoder);
    }


// ---------------------------------------------------------7
// AVCEditParser::ProcessAVCBitStream
// Process one input AVC frame, i.e., convert to MDF NAL unit
// (other items were commented in a header).
// ---------------------------------------------------------
//				
void CVedAVCEditImp::ProcessAVCBitStreamL(TDes8& aBuf, TInt& aFrameLen, TInt aDecInfoSize, TBool aFirstFrame)
    {
    
    TUint8* dataBuffer = (TUint8*)(aBuf.Ptr());        

    // Calculate NAL header start offset
    TInt offset = (((aFrameLen - 1) / 4) + 1) * 4;     // Align at 32-bit boundrary
    TInt numNALUnits = 0;        

    if (aFirstFrame)    // There are several NAL units: decoder info and the frame
        {
        // how many bytes used for length
        iFrameLengthBytes = ( dataBuffer[4] & 0x3 ) + 1;
        
        // Index where to read configuration data
        TInt index = 5;     // Skip version and length information                        
        
        TInt numOfSPS = dataBuffer[index] & 0x1f;
        index++;
        
        // Loop all SPS units
        for (TInt i = 0; i < numOfSPS; ++i)
            {
            TInt SPSSize = (dataBuffer[index] << 8) + dataBuffer[index + 1];
            index += 2;
            
            // Set NAL start offset
            dataBuffer[offset + 0] = TUint8(index & 0xff);
            dataBuffer[offset + 1] = TUint8((index >> 8) & 0xff);
            dataBuffer[offset + 2] = TUint8((index >> 16) & 0xff);
            dataBuffer[offset + 3] = TUint8((index >> 24) & 0xff);
            
            // Set NAL size
            dataBuffer[offset + 4] = TUint8(SPSSize & 0xff);
            dataBuffer[offset + 5] = TUint8((SPSSize >> 8) & 0xff);
            dataBuffer[offset + 6] = TUint8((SPSSize >> 16) & 0xff);
            dataBuffer[offset + 7] = TUint8((SPSSize >> 24) & 0xff);
            
            offset += 8;
            index += SPSSize;
            numNALUnits++;
            }
            
        TInt numOfPPS = dataBuffer[index];
        index++;
        
        // Loop all PPS units
        for (TInt i = 0; i < numOfPPS; ++i)
            {
            TInt PPSSize = (dataBuffer[index] << 8) + dataBuffer[index + 1];
            index += 2;
            
            // Set NAL start offset
            dataBuffer[offset + 0] = TUint8(index & 0xff);
            dataBuffer[offset + 1] = TUint8((index >> 8) & 0xff);
            dataBuffer[offset + 2] = TUint8((index >> 16) & 0xff);
            dataBuffer[offset + 3] = TUint8((index >> 24) & 0xff);
            
            // Set NAL size
            dataBuffer[offset + 4] = TUint8(PPSSize & 0xff);
            dataBuffer[offset + 5] = TUint8((PPSSize >> 8) & 0xff);
            dataBuffer[offset + 6] = TUint8((PPSSize >> 16) & 0xff);
            dataBuffer[offset + 7] = TUint8((PPSSize >> 24) & 0xff);
            
            offset += 8;
            index += PPSSize;
            numNALUnits++;
            }
        
        TInt totalFrameSize = aFrameLen;
        TInt currentProcessed = aDecInfoSize + 4; // skip DCR & length
        TUint8* frameLenPtr = const_cast<TUint8*>(aBuf.Ptr()) + aDecInfoSize;
        
        TInt frameSize = 0;
        
        // loop all slice NAL units
        while (currentProcessed < totalFrameSize)
            {                                        
                
            // Set the NAL start offset
            dataBuffer[offset + 0] = TUint8(currentProcessed & 0xff);
            dataBuffer[offset + 1] = TUint8((currentProcessed >> 8) & 0xff);
            dataBuffer[offset + 2] = TUint8((currentProcessed >> 16) & 0xff);
            dataBuffer[offset + 3] = TUint8((currentProcessed >> 24) & 0xff);
            
            frameSize = (frameLenPtr[0] << 24) + (frameLenPtr[1] << 16) +
                        (frameLenPtr[2] << 8) + frameLenPtr[3];
            
            // Set the NAL size
            dataBuffer[offset + 4] = TUint8(frameSize & 0xff);
            dataBuffer[offset + 5] = TUint8((frameSize >> 8) & 0xff);
            dataBuffer[offset + 6] = TUint8((frameSize >> 16) & 0xff);
            dataBuffer[offset + 7] = TUint8((frameSize >> 24) & 0xff);
            
            frameLenPtr += (4 + frameSize);
            currentProcessed += (4 + frameSize);
            offset += 8;    
            numNALUnits++;
            
            }
        
        // Set Number of NAL units
        dataBuffer[offset + 0] = TUint8(numNALUnits & 0xff);
        dataBuffer[offset + 1] = TUint8((numNALUnits >> 8) & 0xff);
        dataBuffer[offset + 2] = TUint8((numNALUnits >> 16) & 0xff);
        dataBuffer[offset + 3] = TUint8((numNALUnits >> 24) & 0xff);
        
        aFrameLen = offset + 4;
        }
    else
        {  // process just the frame

        TInt totalFrameSize = aFrameLen;
        TInt currentProcessed = 4;  // skip length
        TUint8* frameLenPtr = const_cast<TUint8*>(aBuf.Ptr());
        
        TInt frameSize = 0;
        
        // loop all slice NAL units
        while (currentProcessed < totalFrameSize)
            {
            // Set the NAL start offset
            dataBuffer[offset + 0] = TUint8(currentProcessed & 0xff);
            dataBuffer[offset + 1] = TUint8((currentProcessed >> 8) & 0xff);
            dataBuffer[offset + 2] = TUint8((currentProcessed >> 16) & 0xff);
            dataBuffer[offset + 3] = TUint8((currentProcessed >> 24) & 0xff);
            
            frameSize = (frameLenPtr[0] << 24) + (frameLenPtr[1] << 16) +
                        (frameLenPtr[2] << 8) + frameLenPtr[3];
            
            // Set the NAL size
            dataBuffer[offset + 4] = TUint8(frameSize & 0xff);
            dataBuffer[offset + 5] = TUint8((frameSize >> 8) & 0xff);
            dataBuffer[offset + 6] = TUint8((frameSize >> 16) & 0xff);
            dataBuffer[offset + 7] = TUint8((frameSize >> 24) & 0xff);
            
            frameLenPtr += (4 + frameSize);
            currentProcessed += (4 + frameSize);
            offset += 8;    
            numNALUnits++;
            }
               
        // Number of NAL units
        dataBuffer[offset + 0] = TUint8(numNALUnits & 0xff);
        dataBuffer[offset + 1] = TUint8((numNALUnits >> 8) & 0xff);
        dataBuffer[offset + 2] = TUint8((numNALUnits >> 16) & 0xff);
        dataBuffer[offset + 3] = TUint8((numNALUnits >> 24) & 0xff);
        
        aFrameLen = offset + 4;
        }
    //iDataLength = iCurrentFrameLength;
    }
    
// ---------------------------------------------------------
// CVedAVCEditImp::GetMaxAVCFrameBuffering
// Calculate maximum amount of buffered AVC frames 
// (other items were commented in a header).
// ---------------------------------------------------------
//				
TInt CVedAVCEditImp::GetMaxAVCFrameBuffering(TInt aLevel, TSize aResolution)
    {

    TReal maxDPB = 0.0;    
    switch (aLevel)
        {
        case 11:        
            maxDPB = 337.5;        
            break;
                        
        case 12:
            maxDPB = 891.0;
            break;
        
        case 10:
        case 101:
        default:
            maxDPB = 148.5;
            break;
        }
    
    TInt mbWidth = aResolution.iWidth / 16;
    TInt mbHeight = aResolution.iHeight / 16;
    
    TInt maxDPBSize = TInt( ( TReal(1024.0) * maxDPB ) / ( TReal(mbWidth*mbHeight*384.0) ) );
 
    maxDPBSize = min(maxDPBSize, 16);
 
    return maxDPBSize;
    }
    
// ---------------------------------------------------------
// CVedAVCEditImp::GetLevel   
// Get input bitstream level from SPS
// ---------------------------------------------------------
//
TInt CVedAVCEditImp::GetLevel(TDesC8& aBuf, TInt& aLevel)
    {
    TUint8* buffer = (TUint8*)(aBuf.Ptr());

    TInt index = 5;     // Skip version and length information                        
    
#ifdef _DEBUG
    TInt numOfSPS = buffer[index] & 0x1f;
    VPASSERT(numOfSPS == 1);
#endif
    
    index++;
    
    TUint SPSSize = (buffer[index] << 8) + buffer[index + 1];
    index += 2;
    
    TInt error = avcdParseLevel(iAvcDecoder, (void*)&buffer[index], &SPSSize, aLevel);
    
    if (error != KErrNone)
        return error;

    return KErrNone;
    }


// ---------------------------------------------------------
// CVedAVCEditImp::GetResolution   
// Get input bitstream resolution from SPS
// ---------------------------------------------------------
//
TInt CVedAVCEditImp::GetResolution(TDesC8& aBuf, TSize& aResolution)
{

    TUint8* buffer = (TUint8*)(aBuf.Ptr());

    TInt index = 5;     // Skip version and length information                        
    
#ifdef _DEBUG
    TInt numOfSPS = buffer[index] & 0x1f;
    VPASSERT(numOfSPS == 1);
#endif
    
    index++;
    
    TUint SPSSize = (buffer[index] << 8) + buffer[index + 1];
    index += 2;
    
    TInt error = avcdParseResolution(iAvcDecoder, (void*)&buffer[index], &SPSSize, 
                                     aResolution.iWidth, aResolution.iHeight);
    
    if (error != KErrNone)
        return error;

    return KErrNone;
}



#ifdef VIDEOEDITORENGINE_AVC_EDITING
// ---------------------------------------------------------
// AVCEditParser::SaveAVCDecoderConfigurationRecordL
// Saves SPS/PPS Nal units from AVCDecoderConfigurationRecord
// (other items were commented in a header).
// ---------------------------------------------------------
//
void CVedAVCEditImp::SaveAVCDecoderConfigurationRecordL(TDes8& aBuf, TBool aFromEncoder)
    {

    TUint8* buffer = (TUint8*)(aBuf.Ptr());

    TInt index = 5;     // Skip version and length information                        
    
    TInt numOfSPS = buffer[index] & 0x1f;
    index++;
    
    // Loop all SPS units
    for (TInt i = 0; i < numOfSPS; ++i)
        {
        TUint SPSSize = (buffer[index] << 8) + buffer[index + 1];
        index += 2;
        
        // feed NAL for saving to ParseParameterSet()        
        User::LeaveIfError( ParseParameterSet( (void*)&buffer[index], &SPSSize, aFromEncoder ) );
        index += SPSSize;                        
        
        }
        
    TInt numOfPPS = buffer[index];
    index++;
    
    // Loop all PPS units
    for (TInt i = 0; i < numOfPPS; ++i)
        {
        TUint PPSSize = (buffer[index] << 8) + buffer[index + 1];
        index += 2;
        
        // feed NAL for saving to ParseParameterSet()        
        User::LeaveIfError( ParseParameterSet( (void*)&buffer[index], &PPSSize, aFromEncoder ) );
        index += PPSSize;
        
        }
  
    }

// ---------------------------------------------------------
// CVedAVCEditImp::ConvertAVCHeader  
// Convert AVC specific decoder config info to 
// AVCDecoderConfigurationRecord -format
// (other items were commented in a header).
// ---------------------------------------------------------
//				
void CVedAVCEditImp::ConvertAVCHeaderL( TDesC8& aSrcBuf, TDes8& aDstBuf )
    {
    
	TUint8* inputPtr = (TUint8*)(aSrcBuf.Ptr());
    TUint8* outputPtr = (TUint8*)(aDstBuf.Ptr());
    TUint8* spsPtr;
    TUint8* ppsPtr;
    
    TUint numSPS = 0;
    TUint numPPS = 0;
    
    TUint totalSPSLength = 0;
    TUint totalPPSLength = 0;
    
    TUint headerLength = aSrcBuf.Length();
    TUint endIndex = headerLength;
    
   	TInt nalType = 0;
   	TUint nalLength;
   	TUint nalIndex;
    TUint nalOffset;        

   	// Allocate memory for the temporary buffers
   	HBufC8* temp1 = (HBufC8*) HBufC8::NewLC(1000);
   	HBufC8* temp2 = (HBufC8*) HBufC8::NewLC(5000);

    spsPtr = const_cast<TUint8*>( temp1->Des().Ptr() );
    ppsPtr = const_cast<TUint8*>( temp2->Des().Ptr() );

    TUint numNalUnits = inputPtr[endIndex-4] + (inputPtr[endIndex-3]<<8) + (inputPtr[endIndex-2]<<16) + (inputPtr[endIndex-1]<<24);

	// Move endIndex to point to the first NAL unit's offset information    
    endIndex = headerLength - numNalUnits*8 - 4;
    
	nalIndex = 0;    
	
	TUint8* copyPtr = inputPtr;

	while (nalIndex < numNalUnits)
        {
    	nalIndex++;
    	
    	TInt tmp1 = inputPtr[endIndex++];
        TInt tmp2 = inputPtr[endIndex++]<<8;
        TInt tmp3 = inputPtr[endIndex++]<<16;
        TInt tmp4 = inputPtr[endIndex++]<<24;    	    	

    	nalOffset = tmp1 + tmp2 + tmp3 + tmp4;
    	
    	tmp1 = inputPtr[endIndex++];
        tmp2 = inputPtr[endIndex++]<<8;
        tmp3 = inputPtr[endIndex++]<<16;
        tmp4 = inputPtr[endIndex++]<<24;
    	
    	nalLength = tmp1 + tmp2 + tmp3 + tmp4;    	

	   	nalType = inputPtr[nalOffset] & 0x1F;
  			
	   	if(nalType == 7)
		    {
	   		numSPS++;	   			   			   		

		    // First store the SPS unit length with two bytes
   			spsPtr[totalSPSLength] = (nalLength >> 8) & 0xFF;
    		spsPtr[totalSPSLength+1] = nalLength & 0xFF;	    			    		    		    		
			
			// Copy the SPS unit to the buffer    
			Mem::Copy(&spsPtr[totalSPSLength+2], copyPtr , nalLength);
										
			totalSPSLength += nalLength + 2;	// Two more for the size
		    }
  		else if(nalType == 8)
   		    {
			numPPS++;

		    // First store the SPS unit length with two bytes
   			ppsPtr[totalPPSLength] = (nalLength >> 8) & 0xFF;
    		ppsPtr[totalPPSLength+1] = nalLength & 0xFF;
	
			// Copy the SPS unit to the buffer    
			Mem::Copy(&ppsPtr[totalPPSLength+2], copyPtr , nalLength);
	
			totalPPSLength += nalLength + 2;	// Two more for the size
	   	    }
		else
   		    {
			// [KW]: Check later if this is an error!!!
   		    }
   		
   		copyPtr += nalLength;
        }
    
	// When the header has been parsed, form the AVCDecoderConfigurationRecord
	outputPtr[0] = 0x01;	// configurationVersion 
	outputPtr[1] = 0x42;	// Profile indicator
	// Profile compatibility, i.e. all 4 constrain set flags + reserved 4 zero bits
	outputPtr[2] = 0x80;	// Bitstream obeys all baseline constraints
	if ( iOutputLevel == 101 )
	{
		// For level 1b, the 4th bit shall be == 1, otherwise it must be zero
		outputPtr[2] |= 0x10;
	} 
	else
	{
	    outputPtr[2] &= 0xEF;   
	}
	
	outputPtr[3] = (iOutputLevel == 101) ? 11 : iOutputLevel;  // level
		
	outputPtr[4] = 0x03;	// lengthSizeMinusOne		
    outputPtr[4] |= 0x0FC; // 6 reserved bits (all 1)	
		
	outputPtr[5] = numSPS;		
    outputPtr[5] |= 0xE0;  // 3 reserved bits (all 1)

	TInt len = 6;
	
	// Copy the SPS unit(s) to the buffer    
	Mem::Copy(&outputPtr[6], spsPtr , totalSPSLength);
	
	len += totalSPSLength;

	outputPtr[6+totalSPSLength] = numPPS;
	
	len += 1;
		
	// Copy the PPS unit(s) to the buffer    
	Mem::Copy(&outputPtr[6+totalSPSLength+1], ppsPtr , totalPPSLength);
	
	len += totalPPSLength;	
	
	aDstBuf.SetLength(len);
	
	CleanupStack::Pop(2);
	
	// Free the temporary buffers
	delete temp1;
	delete temp2;	
}


// ---------------------------------------------------------
// CVedAVCEditImp::ParseOneNAL  
// Saves one SPS/PPS NAL unit for later use
// ---------------------------------------------------------
//				
TInt CVedAVCEditImp::ParseParameterSet(void *aNalUnitData, TUint* aNalUnitLength, TBool aFromEncoder)
    {
    	TInt retCode;
    	
		// Pass the information about the frame origin to the decoder 
		FrameIsFromEncoder(iAvcDecoder, aFromEncoder);
		// Just call the decoder's parser function
		retCode = avcdParseParameterSet(iAvcDecoder, aNalUnitData, aNalUnitLength);
		
		return retCode;
    }

TInt CVedAVCEditImp::ParseOneNAL(void *aNalUnitData, TUint* aNalUnitLength, TBool aFromEncoder)
    {
    	TInt retCode;
    	
		// Pass the information about the frame origin to the decoder 
		FrameIsFromEncoder(iAvcDecoder, aFromEncoder);
		// Just call the decoder's parser function
		retCode = avcdParseOneNal(iAvcDecoder, aNalUnitData, aNalUnitLength);
		
		return retCode;
    }

// ---------------------------------------------------------
// CVedAVCEditImp::ParseFrame  
// Update slice header information
// ---------------------------------------------------------
//				
TInt CVedAVCEditImp::ParseFrame(HBufC8*& aBuf, TBool aContainsDCR, TBool aFromEncoder)
{
    TUint nalSize;
    TUint nalOrigSize = 0;
	TUint nalLengthSize = 0;
//	TInt nalType;
//	TInt nalRefIdc;
	TInt skip = 0;
	TUint bufferLength = aBuf->Length();
    TPtr8 bufferPtr(aBuf->Des());    
	TUint8* srcPtr = (TUint8*)(bufferPtr.Ptr());
	
	TInt error;
	HBufC8* temp1 = 0;
  	TRAP( error, temp1 = (HBufC8*) HBufC8::NewL(10) );
  	
  	if (error != KErrNone)
  	    return error;
  	  	  	
  	TPtr8 tempPtr(temp1->Des());
//	TUint tmpLength1;
//	TUint tmpLength2;
    TUint8* tempData1;	

  	tempPtr.Append(5);

	// Jump over the AVC decoder information if it's included
    if(aContainsDCR)
    {
		// skip 4 bytes for 
		// configVersion, profile, profile compatibility and Level
		skip += 4;
	
		// skip 1 byte for lengthSizeMinusOne
		skip += 1;
	
		// skip 1 byte for number of sequence parameter sets
		TInt numOfSSP = 0x1F & srcPtr[skip];
		skip += 1;
	
		for (TInt i = 0; i < numOfSSP; i++)
	    {
	      	TInt sspSize = srcPtr[skip]*256 + srcPtr[skip+1];
   			skip += 2;
	        skip += sspSize;
	    }

		TInt numOfPSP = srcPtr[skip];
		skip += 1;

		for (TInt i = 0; i < numOfPSP; i++)
	    {
   			TInt pspSize = srcPtr[skip]*256 + srcPtr[skip+1];
	       	skip += 2;
       		skip += pspSize;
	    }
    }

 	while (skip < bufferLength)
 	{
 	   
 	    TInt retVal = 0; 			    

		nalLengthSize = iNalLengthSize;
		switch (nalLengthSize)
		{
			case 1:
				nalOrigSize = nalSize = srcPtr[skip];
				skip += 1;
				break;
			case 2:
				nalOrigSize = nalSize = (srcPtr[skip] << 8)  + srcPtr[skip+1];
				skip += 2;
				break;
			case 4:
				nalOrigSize = nalSize = (srcPtr[skip] << 24) + (srcPtr[skip+1] << 16) + 
                          (srcPtr[skip+2] << 8) + srcPtr[skip+3];  
				
				skip += 4;
				break;
		}
		
//	   	nalType   = srcPtr[skip] & 0x1F;
//		nalRefIdc = srcPtr[skip] & 0x60;	

		// [KW]: Alloc memory here instead of sequence.cpp
	  	tempData1 = (TUint8*) User::Alloc(nalOrigSize+100);   

		if (tempData1 == 0)
		{
		    User::Free(temp1);
			return KErrNoMemory;
		}

		Mem::Copy(tempData1, &srcPtr[skip], nalOrigSize*sizeof(TUint8));

		Mem::FillZ(&tempData1[nalOrigSize], 100*sizeof(TUint8));

		// Call ParseOneNaL function
		retVal = ParseOneNAL(tempData1, &nalSize, aFromEncoder);
		
		if (retVal != KErrNone)
		{
            User::Free(tempData1);
        	User::Free(temp1);
		    return retVal;   
		}
		
		// Copy data back to the srcPtr
		Mem::Copy(&srcPtr[skip],tempData1,nalOrigSize*sizeof(TUint8));
		
//		tmpLength1 = aBuf->Length();
		if(nalSize > nalOrigSize)
		{
			TUint diff = nalSize - nalOrigSize;
		
			for (TInt i=0; i<diff; i++)
			{
				tempPtr.Delete(0,1);
				tempPtr.Append(tempData1[nalOrigSize+i]);
//				tmpLength2 = tempPtr.Length();

				// Insert byte(s) into the buffer
    			if((bufferPtr.Length() + 1) > bufferPtr.MaxLength()) 
    			{
        			// extend buffer size
		        	TUint newSize = bufferPtr.Length() + 1;
		        	
        			// round up to the next full kilobyte       
        			newSize = (newSize + 1023) & (~1023);
        			TRAP(error, (aBuf = aBuf->ReAllocL(newSize)) );
        
        			if (error != KErrNone)
        			{
        				User::Free(tempData1);
        				User::Free(temp1);
            			return error;
        			}
        
        			bufferPtr.Set(aBuf->Des());
    			}        
    			
				bufferPtr.Insert(skip+nalOrigSize+i,tempPtr);
//				tmpLength1 = aBuf->Length();
			}
			bufferLength += diff;
		}
		else if(nalSize < nalOrigSize)
		{
			TUint diff = nalOrigSize - nalSize;
		
			// Delete diff bytes from the buffer
			bufferPtr.Delete(skip+nalOrigSize-diff,diff);
			
			bufferLength -= diff;
		}
		
		// Update the NAL unit's size information in the buffer
        srcPtr[skip-4] = TUint8((nalSize >> 24) & 0xff);
        srcPtr[skip-3] = TUint8((nalSize >> 16) & 0xff);
        srcPtr[skip-2] = TUint8((nalSize >> 8) & 0xff);
        srcPtr[skip-1] = TUint8(nalSize & 0xff);

		// Free the temporary data        
        User::Free(tempData1);
		
		skip += nalSize;
    }
    
    User::Free(temp1);
    
    return KErrNone;
}

// ---------------------------------------------------------
// CVedAVCEditImp::ConstructAVCDecoderConfigurationRecordL   
// Constructs AVCDecoderConfigurationRecord for output
// ---------------------------------------------------------
//				
void CVedAVCEditImp::ConstructAVCDecoderConfigurationRecordL( TDes8& aDstBuf )
    {
    
    TUint8* outputPtr = (TUint8*)(aDstBuf.Ptr());
    TUint8* spsPtr;
    TUint8* ppsPtr;
    
    TUint numSPS = 0;
    TUint numPPS = 0;
    
    TUint totalSPSLength = 0;
    TUint totalPPSLength = 0;
    
    TInt i;
    TUint spsLength;
    TUint ppsLength;
	TInt len = 6;
	TUint8* copyPtr;
         

   	// Allocate memory for the temporary buffers
   	HBufC8* temp1 = (HBufC8*) HBufC8::NewLC(1000);
   	HBufC8* temp2 = (HBufC8*) HBufC8::NewLC(5000);

    spsPtr = const_cast<TUint8*>( temp1->Des().Ptr() );
    ppsPtr = const_cast<TUint8*>( temp2->Des().Ptr() );        

    numSPS = ReturnNumSPS(iAvcDecoder);
    numPPS = ReturnNumPPS(iAvcDecoder);
    
    for (i=0; i<numSPS; i++)
    {
    	copyPtr = ReturnSPSSet(iAvcDecoder,i,&spsLength);

	    // First store the SPS unit length with two bytes
		spsPtr[totalSPSLength] = (spsLength >> 8) & 0xFF;
  		spsPtr[totalSPSLength+1] = spsLength & 0xFF;

		// Copy the SPS unit to the buffer    
		Mem::Copy(&spsPtr[totalSPSLength+2], copyPtr , spsLength);
		
		totalSPSLength += spsLength + 2;	// Two more for the size
    }
    
    
    for (i=0; i<numPPS; i++)
    {
    	copyPtr = ReturnPPSSet(iAvcDecoder,i,&ppsLength);

	    // First store the PPS unit length with two bytes
		ppsPtr[totalPPSLength] = (ppsLength >> 8) & 0xFF;
  		ppsPtr[totalPPSLength+1] = ppsLength & 0xFF;

		// Copy the PPS unit to the buffer    
		Mem::Copy(&ppsPtr[totalPPSLength+2], copyPtr , ppsLength);

		totalPPSLength += ppsLength + 2;	// Two more for the size
    }
    
    
    
	// When the header has been parsed, form the AVCDecoderConfigurationRecord
	outputPtr[0] = 0x01;	
	outputPtr[1] = 0x42;	// Profile indicator, baseline profile
	
	// Profile compatibility, i.e. all 4 constrain set flags + reserved 4 zero bits
	outputPtr[2] = 0x80;	// Bitstream obeys all baseline constraints
	if ( iOutputLevel == 101 )
	{
		// For level 1b, the 4th bit shall be == 1, otherwise it must be zero
		outputPtr[2] |= 0x10;		
	} 
	else
	{
	    outputPtr[2] &= 0xEF;
	}
	
	outputPtr[3] = (iOutputLevel == 101) ? 11 : iOutputLevel;  // level
	outputPtr[4] = 0x03;	// lengthSizeMinusOne		
	outputPtr[5] = numSPS;		

	
	// Copy the SPS unit(s) to the buffer    
	Mem::Copy(&outputPtr[6], spsPtr , totalSPSLength);
	
	len += totalSPSLength;

	outputPtr[6+totalSPSLength] = numPPS;
	
	len += 1;
		
	// Copy the PPS unit(s) to the buffer    
	Mem::Copy(&outputPtr[6+totalSPSLength+1], ppsPtr , totalPPSLength);
	
	len += totalPPSLength;	
	
	aDstBuf.SetLength(len);
	
	CleanupStack::Pop(2);
	// Free the temporary buffers
	delete temp1;
	delete temp2;	
    }
    

// ---------------------------------------------------------
// CVedAVCEditImp::EncodeUntilIDR   
// Returns whether frames have to be encoded until next IDR frame
// ---------------------------------------------------------
//	
TBool CVedAVCEditImp::EncodeUntilIDR()
{
    if (iAvcDecoder)
    {
		return (ReturnEncodeUntilIDR(iAvcDecoder));
    }
    else
    {
    	return EFalse;
    }
}


// ---------------------------------------------------------
// CVedAVCEditImp::IsNALUnitIDR   
// Returns whether passed frame is an IDR frame
// ---------------------------------------------------------
//
TBool CVedAVCEditImp::IsNALUnitIDR( TDes8& aNalBuf )
{
    TUint8* bufferPtr = (TUint8*)(aNalBuf.Ptr());
	
	// skip 4 bytes of length information
	if((bufferPtr[4] & 0x1F) == 5)
		return ETrue;
	else
		return EFalse;
}

// ---------------------------------------------------------
// CVedAVCEditImp::StoreCurrentPPSId
// Stores the PPS id of passed frame for later use
// ---------------------------------------------------------
//
void CVedAVCEditImp::StoreCurrentPPSId( TDes8& aNalBuf )
{
    TUint8* bufferPtr = (TUint8*)(aNalBuf.Ptr());
    TUint bufferLength = aNalBuf.Length();

	switch (iNalLengthSize)
	{
		case 1:
			bufferLength = bufferPtr[0];
			bufferPtr += 1;
			break;
		case 2:
			bufferLength = (bufferPtr[0] << 8)  + bufferPtr[1];
			bufferPtr += 2;
			break;
		case 4:
			bufferLength = (bufferPtr[0] << 24) + (bufferPtr[1] << 16) + 
                         (bufferPtr[2] << 8) + bufferPtr[3];  
			
			bufferPtr += 4;
			break;
	}
	avcdStoreCurrentPPSId(iAvcDecoder, bufferPtr, bufferLength);
}

// ---------------------------------------------------------
// CVedAVCEditImp::GenerateNotCodedFrame
// Generates a not coded (empty) frame
// ---------------------------------------------------------
//
TInt CVedAVCEditImp::GenerateNotCodedFrame( TDes8& aNalBuf, TUint aFrameNumber )
{
    TUint8* bufferPtr = (TUint8*)(aNalBuf.Ptr());
    TUint bufferLength = aNalBuf.Length();
    TInt frameLength = 0;
	
  	frameLength = avcdGenerateNotCodedFrame(iAvcDecoder, bufferPtr, bufferLength, aFrameNumber);
	
	if(frameLength > 0)	
	{
    	TInt i;
    
  		// Make room for iNalLengthSize bytes of length information to the start
  		for (i=frameLength-1; i>=0; i--)
  		{
  			bufferPtr[i+iNalLengthSize] = bufferPtr[i];
  		}

		// Add the NAL size information to the buffer
		switch (iNalLengthSize)
		{
			case 1:
    			bufferPtr[0] = TUint8(frameLength & 0xff);
    			frameLength++;
				break;
			case 2:
		    	bufferPtr[0] = TUint8((frameLength >> 8) & 0xff);
    			bufferPtr[1] = TUint8(frameLength & 0xff);
    			frameLength += 2;
				break;
			case 4:
		    	bufferPtr[0] = TUint8((frameLength >> 24) & 0xff);
    			bufferPtr[1] = TUint8((frameLength >> 16) & 0xff);
    			bufferPtr[2] = TUint8((frameLength >> 8) & 0xff);
    			bufferPtr[3] = TUint8(frameLength & 0xff);
    			frameLength += 4;
				break;
		}
		
	  	return frameLength;
	}
	else
		return 0;
}

// ---------------------------------------------------------
// CVedAVCEditImp::ModifyFrameNumber
// Modifies the frame number of input NAL unit
// ---------------------------------------------------------
//
void CVedAVCEditImp::ModifyFrameNumber( TDes8& aNalBuf, TUint aFrameNumber )
{
    TUint8* bufferPtr = (TUint8*)(aNalBuf.Ptr());
    TUint bufferLength = aNalBuf.Length();

	switch (iNalLengthSize)
	{
		case 1:
			bufferLength = bufferPtr[0];
			bufferPtr += 1;
			break;
		case 2:
			bufferLength = (bufferPtr[0] << 8)  + bufferPtr[1];
			bufferPtr += 2;
			break;
		case 4:
			bufferLength = (bufferPtr[0] << 24) + (bufferPtr[1] << 16) + 
                         (bufferPtr[2] << 8) + bufferPtr[3];  
			
			bufferPtr += 4;
			break;
	}
	
	if (bufferPtr[0]==0x01 && bufferPtr[1]==0x42)
		return;
	
	avcdModifyFrameNumber(iAvcDecoder, bufferPtr, bufferLength, aFrameNumber);
}

#endif

// End of file   


