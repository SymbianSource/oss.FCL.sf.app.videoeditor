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
* Prediction error block decoding functions.
*
*/



/*
 * Includes 
 */
#include "h263dConfig.h"
#include "decmbdct.h"
#include "decblock.h"
#include "block.h"
#include "viddemux.h"
/* MVE */
#include "MPEG4Transcoder.h"

/*
 * Globals
 */

/* New chroma QP values in MQ mode. See Table T.1/H.263 */
static const u_char dmdMQChromaTab[32] = {0,1,2,3,4,5,6,6,7,8,9,9,10,10,
                                          11,11,12,12,12,13,13,13,14,14,
                                          14,14,14,15,15,15,15,15};


/*
 * Global functions
 */


/* {{-output"dmdGetAndDecodeIMBBlocks.txt"}} */
/*
 * dmdGetAndDecodeIMBBlocks
 *    
 *
 * Parameters:
 *    param                   parameters needed in this function
 *
 * Function:
 *    This function gets the DCT coefficients of an INTRA macroblock
 *    from the bitstream, reconstructs the corresponding
 *    pixel-domain blocks and puts the blocks to the output frame.
 *
 * Returns:
 *    >= 0                       the function was successful
 *    < 0                        an error occured when accessing bit buffer
 *
 */

int dmdGetAndDecodeIMBBlocks(
   dmdIParam_t *param, CMPEG4Transcoder *hTranscoder)
/* {{-output"dmdGetAndDecodeIMBBlocks.txt"}} */
{
   bibBuffer_t 
      *inBuffer;        /* Input bit buffer instance */

   TBool getDecodedFrame;


   int 
      cWidth,           /* Chrominance image width in pixels */
      block[64],        /* Temporal 8 x 8 block of pixels */
      i,                /* Loop variable */
      chrQuant,         /* Quantization parameter for chroma */
      bitErrorIndication = 0, 
                        /* Carries bit error indication information returned
                           by the video demultiplexer module */
      ret = 0;          /* Used to check return values of function calls */

   u_char 
      *yBlockInFrame;   /* Points to top-left corner of the current block
                           inside the current frame */

   inBuffer = param->inBuffer;

   getDecodedFrame = param->iGetDecodedFrame;


   yBlockInFrame = param->yMBInFrame;
   cWidth = param->yWidth / 2;


   /* Luminance blocks */
   for (i=0; i<4; i++) 
     {
         /* MVE */
        hTranscoder->BeginOneBlock(i);
         
        bitErrorIndication = 0;
         
         /* Get DCT coefficients */
         ret = vdxGetIntraDCTBlock(inBuffer, vdxIsYCoded(param->cbpy, i + 1), 
             block, &bitErrorIndication, param->fMQ, param->quant);
         if ( ret < 0 )
             return DMD_ERR;
         else if ( ret == VDX_OK_BUT_BIT_ERROR )
             goto corruptedMB;
         
         /* MVE */
         hTranscoder->AddOneBlockDataToMB(i, block);             
         if(getDecodedFrame || hTranscoder->NeedDecodedYUVFrame()) // we need the YUV frames.
         {

             dblIdctAndDequant(block, param->quant, 1);
             blcBlockToFrame(block, yBlockInFrame, param->yWidth);
             yBlockInFrame += 8;
             if (i & 1)
                 yBlockInFrame += 8 * param->yWidth - 16;
         }

   } /* for (y blocks) */

    /* MVE */
    hTranscoder->BeginOneBlock(4);

   /* Chrominance blocks (U) */
   bitErrorIndication = 0;
   /* Find out the value of QP for chrominance block. */
   chrQuant = (param->fMQ)?dmdMQChromaTab[param->quant]:param->quant;

   /* Get DCT coefficients */
   ret = vdxGetIntraDCTBlock(inBuffer, vdxIsUCoded(param->cbpc), 
     block, &bitErrorIndication, param->fMQ, chrQuant);
   if ( ret < 0 )
     return DMD_ERR;
   else if ( ret == VDX_OK_BUT_BIT_ERROR )
     goto corruptedMB;

    /* MVE */
    hTranscoder->AddOneBlockDataToMB(4, block);          
    hTranscoder->BeginOneBlock(5);
   if(getDecodedFrame || hTranscoder->NeedDecodedYUVFrame()) 
     {
         dblIdctAndDequant(block, chrQuant, 1);
         blcBlockToFrame(block, param->uBlockInFrame, cWidth);
     }

   /* (V) */
   bitErrorIndication = 0;
   /* Get DCT coefficients */
   ret = vdxGetIntraDCTBlock(inBuffer, vdxIsVCoded(param->cbpc),
     block, &bitErrorIndication, param->fMQ, chrQuant);
   if ( ret < 0 )
     return DMD_ERR;
   else if ( ret == VDX_OK_BUT_BIT_ERROR )
     goto corruptedMB;

    /* MVE */
    hTranscoder->AddOneBlockDataToMB(5, block);          
    if ( hTranscoder->TranscodingOneMB(NULL) != DMD_OK )
        {
        return DMD_ERR;
        }
    if(getDecodedFrame || hTranscoder->NeedDecodedYUVFrame())
     {
         
         dblIdctAndDequant(block, chrQuant, 1);
         blcBlockToFrame(block, param->vBlockInFrame, cWidth);
         
     }

   return DMD_OK;

corruptedMB:

   return DMD_BIT_ERR;
}


/* {{-output"dmdGetAndDecodePMBBlocks.txt"}} */
/*
 * dmdGetAndDecodePMBBlocks
 *    
 *
 * Parameters:
 *    param                      parameters needed in this function
 *
 * Function:
 *    This function gets the DCT coefficients of an INTER macroblock
 *    from the bitstream, reconstructs the corresponding
 *    pixel-domain blocks and adds the blocks to the prediction frame.
 *
 * Returns:
 *    >= 0                       the function was successful
 *    < 0                        an error occured when accessing bit buffer
 *
 */

int dmdGetAndDecodePMBBlocks(
   dmdPParam_t *param, CMPEG4Transcoder *hTranscoder)
/* {{-output"dmdGetAndDecodePMBBlocks.txt"}} */
{
   bibBuffer_t 
      *inBuffer;        /* Input bit buffer instance */


     TBool getDecodedFrame;

        int 
      yWidth,           /* Luminance image width in pixels */
      block[64],        /* Temporal 8 x 8 block of pixels */
      i,                /* Loop variable */
      chrQuant,         /* Quantization parameter for chroma */
      bitErrorIndication = 0, 
                        /* Carries bit error indication information returned
                           by the video demultiplexer module */
      ret = 0;          /* Used to check return values of function calls */

   u_char 
      *yBlockInFrame;   /* Points to top-left corner of the current block
                           inside the current frame */

   inBuffer = param->inBuffer;


   getDecodedFrame = param->iGetDecodedFrame;


   yWidth = param->uvWidth * 2;
   yBlockInFrame = param->currYMBInFrame;


   int fEscapeCodeUsed = 0;

   /* Luminance blocks */
   for (i = 0; i < 4; i++) {
   
         hTranscoder->BeginOneBlock(i);

         if (vdxIsYCoded(param->cbpy, i + 1)) 
         {
             /* Get DCT coefficients */
             ret = vdxGetDCTBlock(inBuffer, 0, 0, block, 
                 &bitErrorIndication, 0, param->quant, &fEscapeCodeUsed);
             if ( ret < 0 )
                 return DMD_ERR;
             else if ( ret == VDX_OK_BUT_BIT_ERROR )
                 return DMD_BIT_ERR;
             
             /* MVE */
             hTranscoder->H263EscapeCoding(i, fEscapeCodeUsed);
             hTranscoder->AddOneBlockDataToMB(i, block);             
             if(getDecodedFrame || hTranscoder->NeedDecodedYUVFrame())
             {
                 
                 dblIdctAndDequant(block, param->quant, 0);
                 
                 blcAddBlock(block, yBlockInFrame,
                     yWidth, param->mbPlace,
                     (u_char) param->fAdvancedPrediction, param->diffMB->block[i]);        
             }
         }

         /* MVE */
         else
         {
             hTranscoder->H263EscapeCoding(i, fEscapeCodeUsed);
             hTranscoder->AddOneBlockDataToMB(i, NULL);          
         }

     yBlockInFrame += 8;
     if (i & 1)
       yBlockInFrame += ((/*8 **/ yWidth<<3) - 16);
   } /* for (y blocks) */

   /* Find out the value of QP for chrominance block. */
   chrQuant = param->quant;

   hTranscoder->BeginOneBlock(4);

   /* Chrominance blocks (U) */
   if (vdxIsUCoded(param->cbpc)) 
     {
     /* Get DCT coefficients */
     ret = vdxGetDCTBlock(inBuffer, 0, 0, block, 
             &bitErrorIndication, 0, param->quant, &fEscapeCodeUsed);
         if ( ret < 0 )
             return DMD_ERR;
         else if ( ret == VDX_OK_BUT_BIT_ERROR )
             return DMD_BIT_ERR;
         
         hTranscoder->H263EscapeCoding(4, fEscapeCodeUsed);
         hTranscoder->AddOneBlockDataToMB(4, block);             
         if(getDecodedFrame || hTranscoder->NeedDecodedYUVFrame())
         {  
             
             dblIdctAndDequant(block, chrQuant,0);
             blcAddBlock(block, param->currUBlkInFrame, param->uvWidth, 0, 0, 0);
         }
   }

   else
   {
         hTranscoder->H263EscapeCoding(4, fEscapeCodeUsed);
         hTranscoder->AddOneBlockDataToMB(4, NULL);          
   }

   hTranscoder->BeginOneBlock(5);

   /* (V) */
   if (vdxIsVCoded(param->cbpc)) 
     {
         /* Get DCT coefficients */
         ret = vdxGetDCTBlock(inBuffer, 0, 0, block, 
             &bitErrorIndication, 0, param->quant, &fEscapeCodeUsed);
         if ( ret < 0 )
             return DMD_ERR;
         else if ( ret == VDX_OK_BUT_BIT_ERROR )
             return DMD_BIT_ERR;
         
         /* MVE */
         hTranscoder->H263EscapeCoding(5, fEscapeCodeUsed);
         hTranscoder->AddOneBlockDataToMB(5, block);             
         if(getDecodedFrame || hTranscoder->NeedDecodedYUVFrame())
         {
         
             dblIdctAndDequant(block, chrQuant,0);
             blcAddBlock(block, param->currVBlkInFrame, param->uvWidth, 0, 0, 0);
         }
   }
   else
   {
         hTranscoder->H263EscapeCoding(5, fEscapeCodeUsed);
         hTranscoder->AddOneBlockDataToMB(5, NULL);          
   }

   if ( hTranscoder->TranscodingOneMB(param) != TX_OK )
    {
    return DMD_ERR;
    }
   return DMD_OK;
   
}

// End of File
