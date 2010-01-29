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
* Video decoder control interface.
*
*/



/*
 * Includes
 */

/* Include the definition header for the active build target */
#include "h263dConfig.h"
/* Include the header for this file */
#include "h263dntc.h"
/* All other inclusions */
#include "biblin.h"
#include "h263dapi.h"
#include "vde.h"
#include "vdemain.h"
#include "vdeti.h"
#include "biblin.h"
/* MVE */
//#include "InnerTranscoder.h"
#include "core.h"
#include "MPEG4Transcoder.h"

/* 
 * Constants 
 */
const TUint KH263StartCodeLength = 3;  // H.263 picture start code length
const TUint KMPEG4StartCodeLength = 4; // MPEG4 picture start code length

/*
* Global functions
*/

/* {{-output"h263dClose.txt"}} */
/*
* h263dClose
*    
*
* Parameters:
*    hInstance                  instance handle
*
* Function:
*    This function closes an H.263 decoder instance.
*
* Returns:
*    Nothing
*/

void H263D_CC h263dClose(h263dHInstance_t hInstance)
/* {{-output"h263dClose.txt"}} */
{
    if (hInstance)
        vdeShutDown(hInstance);
}


/* {{-output"h263dDecodeFrame.txt"}} */
/*
* h263dDecodeFrameL
*    
*
* Parameters:
*    hInstance                  instance data
*    pStreamBuffer              pointer to input data (>= 1 video frames)
*    streamBufferSize           size of pStreamBuffer
*    fFirstFrame                pointer to flag telling if the very first
*                               frame is being decoded
*    frameSize                  If non-NULL, frameSize is used to return 
*                               the number of bytes that were decoded 
*                               to produce the reconstructed output frame.
*
* Function:
*    This function decodes the bitstream until it gets at least one decoded
*    frame. It also shows the resulting frames. 
*    In addition, the function handles the parameter updating synchronization.
*
* Returns:
*    H263D_OK                   if the function was successful
*    H263D_OK_EOS               if the end of stream has been reached
*    H263D_ERROR                if a fatal error, from which the decoder
*                               cannot be restored, has occured
*    H263D_ERROR_HALTED         the instance is halted, it should be closed
*/

int H263D_CC h263dDecodeFrameL(h263dHInstance_t hInstance,
                               void *pStreamBuffer,
                               void *pOutStreamBuffer,
                               unsigned streamBufferSize,
                               u_char *fFirstFrame,
                               TInt *frameSize,
                               TInt *outframeSize, 
                               vdeDecodeParamters_t *aDecoderInfo
                               )
                                                            
/* {{-output"h263dDecodeFrame.txt"}} */
{
    TInt colorEffect = aDecoderInfo->aColorEffect;
    TInt colorToneU  = aDecoderInfo->aColorToneU;
    TInt colorToneV  = aDecoderInfo->aColorToneV;
    TInt frameOperation = aDecoderInfo->aFrameOperation;
    TInt* trP = aDecoderInfo->aTrP;
    TInt* trD = aDecoderInfo->aTrD; 
    TInt videoClipNumber = aDecoderInfo->aVideoClipNumber; 
    TInt smSpeed = aDecoderInfo->aSMSpeed;
    TInt getDecodedFrame = aDecoderInfo->aGetDecodedFrame; 
    
    bibBuffer_t 
        *buffer = 0;                   /* input buffer */
    
    bibBuffer_t 
        *outbuffer = 0;               /* output buffer */

    bibBuffer_t 
        *tmpbuffer = 0;                /* temporary buffer */

    u_char* tmpbuf=0;

    bibBufferEdit_t
        *bufEdit = 0;
     
    int StartByteIndex=0;
    int StartBitIndex=7;

    int tr = 0;
    int newtr = 0;
    int efftr;
    int numBitsGot;
    int bitsErrorIndication;
    int16 bibError;
    
    int retValue;                  /* return value for vde functions */

    int leaveError = 0;
     
    int16
        errorCode = 0;             /* return code for bib functions */

  /* MVE */
    int startCodeLen = 0;
    CMPEG4Transcoder *hTranscoder = NULL;

    TBool modifyMPEG4Afterwards = EFalse;

    vdeAssert(hInstance);
    vdeAssert(pStreamBuffer);
    vdeAssert(pOutStreamBuffer);
    vdeAssert(streamBufferSize);
    vdeAssert(fFirstFrame);

    vdeInstance_t *vdeinstance = (vdeInstance_t *) hInstance;
    
    /* Create bit buffer */
    buffer = bibCreate(pStreamBuffer, streamBufferSize, &errorCode);
     if (!buffer || errorCode)
         return H263D_ERROR;
     
     /* MVE */

     if ((aDecoderInfo->streamMode==EVedVideoBitstreamModeMPEG4ShortHeader) || (aDecoderInfo->streamMode == EVedVideoBitstreamModeH263) )
         outbuffer = bibCreate(pOutStreamBuffer, streamBufferSize-3, &errorCode);
     else
         outbuffer = bibCreate(pOutStreamBuffer, streamBufferSize-4-3, &errorCode);

     if (!outbuffer || errorCode)
     {
         bibDelete(buffer, &errorCode);
         return H263D_ERROR;
     }
     
     bufEdit = bibBufferEditCreate(&errorCode);
     if (!bufEdit || errorCode)
     {
         bibDelete(outbuffer, &errorCode);
         bibDelete(buffer, &errorCode);
         return H263D_ERROR;
     }   
     
     if(frameOperation==4 /*ENoDecodeNoWrite*/)
     {
         bufEdit->copyMode = CopyNone;
     }
     
     if(colorEffect==1 || colorEffect==2)
     {
         outbuffer->numBytesRead=0;
         outbuffer->size=0;
     }
     
   /* Associate bit buffer with the VDE instance */
     retValue = vdeSetInputBuffer(hInstance, buffer);
     if (retValue < 0)
         goto freeBufferAndReturn;
     
     /* Associate output bit buffer with the VDE instance */
     retValue = vdeSetOutputBuffer(hInstance, outbuffer);
     if (retValue < 0)
         goto freeBufferAndReturn;
     
     retValue = vdeSetBufferEdit(hInstance, bufEdit);
     if (retValue < 0)
         goto freeBufferAndReturn;
     
     retValue = vdeSetVideoEditParams(hInstance, colorEffect,
       getDecodedFrame, colorToneU, colorToneV);
     if (retValue < 0)
         goto freeBufferAndReturn;
     
   /* MVE */
     if (aDecoderInfo->streamMode == EVedVideoBitstreamModeH263 /*H.263*/)
     {
         /* read current TR */
         bibForwardBits(22,buffer); // streamBufferSize is in bytes
         tr = bibGetBits(8,buffer,&numBitsGot, &bitsErrorIndication, &bibError);    
         bibRewindBits(30,buffer,&bibError);
     }

  /* get first frame's QP */
  if (!(*fFirstFrame))
  {
    vdeinstance->iRefQp = aDecoderInfo->aFirstFrameQp;
  }
  vdeinstance->iColorEffect = aDecoderInfo->aColorEffect;
  vdeinstance->iColorToneU = aDecoderInfo->aColorToneU;
  vdeinstance->iColorToneV = aDecoderInfo->aColorToneV;
  
  /* MVE */
  /* one frame is ready, initialize the transcoder */
    TRAP( leaveError, (hTranscoder = CMPEG4Transcoder::NewL(vdeinstance, vdeinstance->inBuffer, vdeinstance->outBuffer)) );
    if ( leaveError != 0 )
    {
        retValue = leaveError;
        goto freeBufferAndReturn;
    }

    if (aDecoderInfo->aGetVideoMode)
    {
        /* we are to determine the bitstream mode */
        aDecoderInfo->aOutputVideoFormat = EVedVideoTypeNoVideo;
        int dummy1, dummy2; // position of the error resillence bit,not used here
        vdtGetVideoBitstreamInfo(buffer, aDecoderInfo, &dummy1, &dummy2);
    }

    /* set transcoding information */
    retValue = hTranscoder->SetTranscoding(aDecoderInfo);
    if ( retValue != TX_OK )
    {
        goto freeBufferAndReturn;
    }
    
    /* before the first frame is decoded, determine the stream type */
    if (*fFirstFrame) {
        retValue = vdeDetermineStreamType(hInstance, hTranscoder);
        if (retValue < 0)
            goto freeBufferAndReturn;
        
    /* MVE */
        /* for the first frame of the bitstream, we may need to construct a new VOS */
        hTranscoder->ConstructVOSHeader(vdeinstance->fMPEG4, aDecoderInfo);
        
        if (vdeinstance->fMPEG4 == 1 )
        {
            StartByteIndex = buffer->getIndex;
            StartBitIndex  = buffer->bitIndex;
            
            /* for MPEG4 (not including shortheader) stuffing bits are inserted at the end of the VOS
            but the index here indicates the position of the stuffing bits
            */
            if (StartBitIndex != 7)
            {
                StartByteIndex += 1;
                StartBitIndex = 7;
            }
            
            // update the output stream size by removing VOS size
            streamBufferSize -= StartByteIndex; 
        }
        
        else if (aDecoderInfo->streamMode == EVedVideoBitstreamModeMPEG4ShortHeader)
        {
            if (aDecoderInfo->aOutputVideoFormat == EVedVideoTypeH263Profile0Level10 ||
                aDecoderInfo->aOutputVideoFormat == EVedVideoTypeH263Profile0Level45)
            {
                // we don't need the VOS header
                StartByteIndex = buffer->getIndex;
                StartBitIndex  = buffer->bitIndex;
                // update the output stream size by removing VOS size
                streamBufferSize -= StartByteIndex; 
            }
            else if (aDecoderInfo->aOutputVideoFormat == EVedVideoTypeMPEG4SimpleProfile &&
                aDecoderInfo->fModeChanged == EFalse)
            {
                // update the output stream size by removing VOS size
                // but we need the original MPEG4 shortheader VOS, which is not yet copied to the output buffer
                streamBufferSize -= buffer->getIndex;
                
            }
        }
        
    }

    /* This may produce multiple output frames, and that is why it is commented
     out. However, it may be useful if the bitstream is corrupted and picture
     start codes are lost
     while (buffer->bitsLeft > 8) { */
     /* Decode frame */

  /* MVE */
  /* The buffer size is changed in h263decoder.cpp
    In the old version, it is fixed to 3, which may be incorrect 
    */
    startCodeLen = (aDecoderInfo->streamMode == EVedVideoBitstreamModeH263)? KH263StartCodeLength : KMPEG4StartCodeLength;
    
    
    
    if ( ((aDecoderInfo->aOutputVideoFormat == EVedVideoTypeMPEG4SimpleProfile) 
            && !(aDecoderInfo->streamMode == EVedVideoBitstreamModeMPEG4ShortHeader))
        || ((aDecoderInfo->streamMode == EVedVideoBitstreamModeMPEG4ShortHeader) && aDecoderInfo->fModeChanged) )
        {
        modifyMPEG4Afterwards = ETrue;
        }

    if (frameOperation==1/*EDecodeAndWrite*/ || frameOperation==2/*EDecodeNoWrite*/)
        {
        if ( vdeinstance->fMPEG4 == 1 )
            {
            buffer->size -= startCodeLen;
            buffer->bitsLeft -= ( startCodeLen << 3 );
            }
        retValue = vdeDecodeFrame(hInstance, StartByteIndex, StartBitIndex, hTranscoder);
        if ( retValue < VDE_OK )
            {
            // negative means fatal error
            goto freeBufferAndReturn;
            }
         // get first frame QP for possible color toning
         if (*fFirstFrame)
         {
           aDecoderInfo->aFirstFrameQp = hTranscoder->GetRefQP();
        }
     }
     else if (frameOperation==3/*EWriteNoDecode*/) 
        {
        /* first reset bit counter to beginning of buffer */
        /* copy input frame as it is */
        bibForwardBits((streamBufferSize-startCodeLen)<<3,buffer); // streamBufferSize is in bytes
         
        if (aDecoderInfo->streamMode == EVedVideoBitstreamModeMPEG4ShortHeader &&
            aDecoderInfo->aOutputVideoFormat == EVedVideoTypeMPEG4SimpleProfile &&
            !(aDecoderInfo->fModeChanged))
            {
            bibRewindBits(32,buffer,&bibError);
            }
         
        /* reset buffer edit to CopyWhole mode */
        bufEdit->copyMode = CopyWhole/*CopyWhole*/;
         
        if ( vdeinstance->fMPEG4 == 1 )
            {
            if (*fFirstFrame)
                {
                buffer->getIndex = buffer->size - startCodeLen;
                buffer->bitIndex = 7;
                buffer->bitsLeft = 24;
                buffer->numBytesRead = buffer->size - startCodeLen;
                }
             
            if ( buffer->numBytesRead > 4 )
                {
                buffer->numBytesRead -= 4;
                buffer->getIndex = buffer->numBytesRead;
                }
            }
         // Reassign pointers so that outbuffer actually has the pointer to (in)buffer's data, 
         // if we have MPEG4 output (not short header) and we need to change e.g. timestamps
         // This eliminates 2 memory copies - otherwise we would copy from buffer to outbuffer to tmpbuffer to outbuffer; 
         // the content is modified when copying from tmpbuffer to outbuffer. 
         // tmpbuffer is needed since we can't modify the content directly in the outbuffer.
         // Now we can copy directly from buffer to outbuffer.
         // In this branch we don't call vdeDecodeFrame at all, but use either CopyEditVopL for MPEG-4 or
         // CopyStream for H.263
         if ( modifyMPEG4Afterwards )
             {
             tmpbuffer = buffer;
             }
         else
             {
             /* copy from buffer to outbuffer. This simulates vdeDecodeFrame with EDecodeAndWrite */
             CopyStream(buffer,outbuffer,bufEdit,StartByteIndex,StartBitIndex);
             }
         retValue=0;
     }
     
     outbuffer->size = outbuffer->getIndex;
     outbuffer->bitsLeft = 0;

     if ( !tmpbuffer )
        {
         /* if tr values are changes, need another buffer */
         /* first create temp buffer in memory */
         tmpbuf = (u_char*) malloc(outbuffer->size + 4);
         if (tmpbuf == NULL) 
         {
             bibDelete(outbuffer, &errorCode);
             bibDelete(buffer, &errorCode);
             bibBufEditDelete(bufEdit, &errorCode);
             return H263D_ERROR;
         }
         tmpbuffer = bibCreate((void*)tmpbuf, outbuffer->size + 4, &errorCode);
         if (!tmpbuffer || errorCode)
         {
            free(tmpbuf);
            bibDelete(outbuffer, &errorCode);
            bibDelete(buffer, &errorCode);
            bibBufEditDelete(bufEdit, &errorCode);
            return H263D_ERROR;
         }
        }
     
   /* MVE */
     if (aDecoderInfo->aOutputVideoFormat != EVedVideoTypeNoVideo)
        {

        if ( modifyMPEG4Afterwards )
            {
            if ( tmpbuffer != buffer )
                {
                // the bitstream was decoded too, need to copy it temporarily from outbuffer to tmpbuffer since it needs to be modified
                bibRewindBits(bibNumberOfFlushedBits(tmpbuffer),tmpbuffer,&bibError);
                bibRewindBits(bibNumberOfFlushedBits(outbuffer),outbuffer,&bibError);
                bibForwardBits(((outbuffer->size)<<3),outbuffer);
                outbuffer->bitsLeft = 0;

                StartByteIndex = 0;
                StartBitIndex = 7;
                
                CopyStream(outbuffer,tmpbuffer,bufEdit,StartByteIndex,StartBitIndex);
                tmpbuffer->baseAddr[tmpbuffer->getIndex] = 0x0;
                tmpbuffer->baseAddr[tmpbuffer->getIndex+1] = 0x0;
                tmpbuffer->baseAddr[tmpbuffer->getIndex+2] = 0x01;
                tmpbuffer->baseAddr[tmpbuffer->getIndex+3] = 0xb6;
                }
             else
                {
                // the tmpbuffer was used as a shortcut to the buffer, no need to copy anything here. 
                tmpbuffer->bitsLeft = 32;
                // the following ones are probably not needed
                tmpbuffer->baseAddr[tmpbuffer->getIndex+4] = 0x0;
                tmpbuffer->baseAddr[tmpbuffer->getIndex+5] = 0x0;
                tmpbuffer->baseAddr[tmpbuffer->getIndex+6] = 0x01;
                tmpbuffer->baseAddr[tmpbuffer->getIndex+7] = 0xb6;
                }
                
           
            bibForwardBits(32,tmpbuffer);
            
            // Copy bitstream part-by-part, possibly modifying it, from tmp to output. 
            // StartByteIndex may be > 0 if there is VOS header in tmpbuffer already. It is always byte-aligned so no need to have StartBitIndex
            TInt skip = 0;
            if ( StartByteIndex > 0 )
                {
                // startByteIndex refers to input buffer. 
                // The skip however should refer to the output buffer. The output header size is in aDecoderInfo
                skip = aDecoderInfo->vosHeaderSize;
                }
            int retVal= CopyEditVop(hInstance, skip, tmpbuffer, aDecoderInfo);
            if(retVal<0)
                {
                retValue = retVal;  //will be handled later
                }
            }
        else
            {
            /* copy input frame while changing TR */
            if((videoClipNumber>0) || (smSpeed!=1000))
                {
                if ((frameOperation==1) || ((frameOperation==2) && !getDecodedFrame))
                    {
                    /* if the output buffer will not be use, leave it as it is! */
                    }
                else
                    {

                    /* get new TR */
                    StartByteIndex = 0;
                    StartBitIndex = 7;
                    outbuffer->bitsLeft = 0;
                    newtr = GetNewTrValue(tr, trP, trD, smSpeed);
                    /* change TR value in output bitstream */
                    /* prepare editing position */
                    if (!bufEdit->editParams)
                        {
                        bufEdit->editParams = (bibEditParams_t *) malloc(sizeof(bibEditParams_t));
                        if (bufEdit->editParams == NULL)
                            {
                            retValue = H263D_ERROR;
                            goto freeBufferAndReturn;
                            }
                        
                        bufEdit->numChanges=1;
                        bufEdit->copyMode = CopyWithEdit; // CopyWithEdit
                        }
                    bufEdit->editParams->curNumBits = bufEdit->editParams->newNumBits = 8; 
                    bufEdit->editParams->StartByteIndex=2; //2;     // starting position for the TR marker 
                    bufEdit->editParams->StartBitIndex=1; //1;          // starting position for the TR marker 
                    bufEdit->editParams->newValue=newtr; 
                    /* copy the input buffer to the output buffer with TR changes */
                    CopyStream(outbuffer,tmpbuffer,bufEdit,StartByteIndex,StartBitIndex);
                    bibRewindBits((streamBufferSize)<<3,buffer,&bibError);
                    /* copy the changed bitstream from tmpbuffer back to buffer */
                    bufEdit->copyMode = CopyWhole; // CopyWhole
                    bibRewindBits(bibNumberOfFlushedBits(outbuffer),outbuffer,&bibError);
                    CopyStream(tmpbuffer,outbuffer,bufEdit,StartByteIndex,StartBitIndex);
                    bibRewindBits((streamBufferSize)<<3,buffer,&bibError);
                    }
                }
            /* update TR values */
            efftr = (videoClipNumber>0 || smSpeed!=1000) ? newtr : tr; 
            *trP = efftr;
            *trD = tr; 
        }
    }
    
    *fFirstFrame = 0;

    if (retValue < 0)
        goto freeBufferAndReturn;   
        /* See the comment above.
        else if (retValue == H263D_OK_EOS)
        break;
   } */

    if (frameSize)
        *frameSize = bibNumberOfFlushedBytes(buffer) + 
        ((bibNumberOfFlushedBits(buffer) % 8 > 0) ? 1 : 0);

    *outframeSize = outbuffer->numBytesRead; 

        
freeBufferAndReturn:

    int bitError = buffer->error;

    if (hTranscoder)
    {
        delete hTranscoder;
        hTranscoder = NULL;
    }
     
    if ( tmpbuffer == buffer )
        {
        // tmpbuffer was a shortcut to inbuffer, set it to NULL to avoid deleting it
        tmpbuffer = NULL;
        }

    int16 deleteError = 0;
    /* Delete bit buffer */
    bibDelete(buffer, &errorCode);
    if ( errorCode )
        {
        deleteError = errorCode;
        }
    
    /* Delete output bit buffer */
    bibDelete(outbuffer, &errorCode);
    if ( errorCode )
        {
        deleteError = errorCode;
        }

    /* Delete tmp bit buffer, if it was used */
    if ( tmpbuffer )
        {
        bibDelete(tmpbuffer, &errorCode);
        if ( errorCode )
            {
            deleteError = errorCode;
            }
        }
    if(tmpbuf)
        free(tmpbuf);
    
    /* Delete bufEdit */
    bibBufEditDelete(bufEdit, &errorCode);
    if ( errorCode )
        {
        deleteError = errorCode;
        }
    if (deleteError || errorCode)
        return H263D_ERROR;

    if ( bitError )
        {
        return H263D_OK_BUT_FRAME_USELESS;
        }

    return retValue;
}


/* {{-output"h263dGetLatestFrame.txt"}} */
/*
* h263dGetLatestFrame
*    
*
* Parameters:
*    hInstance                  instance data
*    
*    ppy, ppu, ppv              used to return Y, U and V frame pointers
*
*    pLumWidth, pLumHeight      used to return luminance image width and height
*
*    pFrameNum                  used to return frame number
*
* Function:
*    This function returns the latest correctly decoded frame
*    (and some side-information).
*
* Returns:
*    H263D_OK                   if the function was successful
*    H263D_ERROR                if a fatal error, from which the decoder
*                               cannot be restored, has occured
*    H263D_ERROR_HALTED         the instance is halted, it should be closed
*/


int H263D_CC h263dGetLatestFrame(
                                                                 h263dHInstance_t hInstance,
                                                                 u_char **ppy, u_char **ppu, u_char **ppv,
                                                                 int *pLumWidth, int *pLumHeight,
                                                                 int *pFrameNum)
                                                                 /* {{-output"h263dGetLatestFrame.txt"}} */
{
    vdeAssert(hInstance);
    vdeAssert(ppy);
    vdeAssert(ppu);
    vdeAssert(ppv);
    vdeAssert(pLumWidth);
    vdeAssert(pLumHeight);
    vdeAssert(pFrameNum);
    
    return vdeGetLatestFrame(hInstance, ppy, ppu, ppv, pLumWidth, pLumHeight,
        pFrameNum);
}



/* {{-output"h263dIsIntra.txt"}} */
/*
* h263dIsIntra
*    
*
* Parameters:
*    hInstance                  handle of instance data
*    frameStart                 pointer to memory chunk containing a frame
*    frameLength                number of bytes in frame
*
* Function:
*    This function returns 1 if the passed frame is an INTRA frame.
*    Otherwise the function returns 0.
*
* Returns:
*    See above.
*/

int h263dIsIntra(
                                 h263dHInstance_t hInstance,
                                 void *frameStart,
                                 unsigned frameLength)
                                 /* {{-output"h263dIsIntra.txt"}} */
{
    vdeAssert(hInstance);
    vdeAssert(frameStart);
    vdeAssert(frameLength);
    
    return vdeIsINTRA(hInstance, frameStart, frameLength);
}

/* {{-output"h263dOpen.txt"}} */
/*
* h263dOpen
*    
*
* Parameters:
*    openParam                  initialization parameters
*
* Function:
*    This function creates and initializes a new H.263 decoder instance.
*
* Returns:
*    an instance handle if the function was successful
*    NULL if an error occured
*/

h263dHInstance_t H263D_CC h263dOpen(h263dOpen_t *openParam)
/* {{-output"h263dOpen.txt"}} */
{
    vdeAssert(openParam);
    
    /* No extra space needs to be allocated after the VDE instance data
    (of type vdeInstance_t) due to the fact that no thread specific
    data is needed. */
    openParam->freeSpace = 0;
    
    return vdeInit(openParam);
}



int GetNewTrValue(int aTrCurOrig, int* aTrPrevNew, int* aTrPrevOrig, int aSMSpeed)
{
    int trCurNew=0;
    int trDiff=0;
    TReal speedFactor = (TReal)aSMSpeed/1000.0;

    trDiff = aTrCurOrig - *aTrPrevOrig;
    if(trDiff==0)  // if corrupt TR values
        trDiff=1;  
    else if(trDiff<0)  // jump in TR values (change of clip or end of limit)
        trDiff = 3;    // arbitrary, for 10 fps default
    // check for slow motion 
    vdeAssert(aSMSpeed);
    if(aSMSpeed!=1000)
        trDiff = (int)((TReal)trDiff/speedFactor + 0.5);
    trCurNew = *aTrPrevNew + trDiff; 
    if(trCurNew>255)
        trCurNew = trCurNew%256;
    return trCurNew;
}



// End of File
