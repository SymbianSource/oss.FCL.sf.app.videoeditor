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
* Prediction error block decoding functions (MPEG-4).
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
 * Global functions
 */

/* {{-output"dmdGetAndDecodeMPEGIMBBlocks.txt"}} */
/*
 * dmdGetAndDecodeMPEGIMBBlocks
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

int dmdGetAndDecodeMPEGIMBBlocks(
   dmdMPEGIParam_t *param, CMPEG4Transcoder *hTranscoder)
/* {{-output"dmdGetAndDecodeMPEGIMBBlocks.txt"}} */
{
   bibBuffer_t *inBuffer;

   bibBuffer_t 
      *outBuffer;        /* Output bit buffer instance */

   bibBufferEdit_t 
      *bufEdit; 

   int colorEffect;
   TBool getDecodedFrame;

   int i, j,
      block[64], cWidth, bitErrorIndication = 0, ret = 0;
   u_char *yBlockInFrame;
   int IntraDC_size, 
        IntraDC_delta,
        DC_coeff;

   inBuffer = param->inBuffer;
   outBuffer = param->outBuffer;
   bufEdit = param->bufEdit;
   colorEffect = param->iColorEffect; 
   getDecodedFrame = param->iGetDecodedFrame;

   if ( param->yMBInFrame )
    {
        yBlockInFrame = param->yMBInFrame +
            ((param->reversible_vlc && param->vlc_dec_direction) ? ((/*8 **/ param->yWidth<<3) + 8) : 0);
    }
   else
    {
        yBlockInFrame = NULL;
    }
   cWidth = (param->yWidth >>1 /*/ 2*/);
   

    /* Loop through the 4 Luminance and the 2 Chrominance blocks */
   for (j=0; j<=5; j++) 
   {

      /* if reversible decoding, the block numbering is reverse */
      if (param->reversible_vlc && param->vlc_dec_direction) 
      {
         i = 5-j;
      }
      else 
      {
         i=j;
      }

      /* MVE */
      hTranscoder->BeginOneBlock(i);

      bitErrorIndication = 0;

      /* Get Intra DC if not switched */
      if(!param->switched) {
         if (param->data_partitioned)
            IntraDC_delta = param->DC[i];
         else {
            ret = vdxGetIntraDC(inBuffer, outBuffer, bufEdit, colorEffect, &(param->StartByteIndex), &(param->StartBitIndex), 
                            i, &IntraDC_size, &IntraDC_delta, &bitErrorIndication);

            if ( ret < 0 )
               return DMD_ERR;
            else if ( ret == VDX_OK_BUT_BIT_ERROR )
               goto corruptedMB;
         }
         block[0] = IntraDC_delta;
      }

      /* Get DCT coefficients */
      if (((i<4) && (vdxIsYCoded(param->cbpy, i + 1))) ||
         ((i==4) && (vdxIsUCoded(param->cbpc))) ||
         ((i==5) && (vdxIsVCoded(param->cbpc))))
      {
         /* if reversible VLC, also the direction of decoding is relevant */
         if (param->reversible_vlc) 
         {
            if (!param->vlc_dec_direction)
               ret = vdxGetRVLCDCTBlock(inBuffer, (!param->switched), 1, block,
                                 &bitErrorIndication);
            else
               ret = vdxGetRVLCDCTBlockBackwards(inBuffer, (!param->switched), 1, block,
                                        &bitErrorIndication);
         } 
         else 
         {
            ret = vdxGetMPEGIntraDCTBlock(inBuffer, (!param->switched), block,
                                  &bitErrorIndication);
         }
         
         if ( ret < 0 )
            return DMD_ERR;
         else if ( ret == VDX_OK_BUT_BIT_ERROR )
            goto corruptedMB;

      } 
      /* if block is not coded */
      else 
      {
         memset(block + (!param->switched), 0, (63 + (param->switched))* sizeof(int));
      }

      /* MVE */
      hTranscoder->AddOneBlockDataToMB(i, block);            

      /* DC/AC prediction: reconstruct the Intra coefficients */
      aicDCACrecon(param->aicData, param->quant,
         param->fTopOfVP, param->fLeftOfVP, param->fBBlockOut,
         i, block, param->currMBNum);

      /* optimized nonlinear inverse quantization for Intra DC */
      DC_coeff = (block[0] *= aicDCScaler(param->quant,(i<4)?1:2));

      hTranscoder->AddOneBlockDCACrecon(i, block);           

      /* Update the AIC module data, with the current MB */
      aicBlockUpdate (param->aicData, param->currMBNum, i, block, 
         param->quant, DC_coeff);
      

      if(getDecodedFrame || hTranscoder->NeedDecodedYUVFrame())
      {
         /* inverse quantization and IDCT */
         dblIdctAndDequant(block, param->quant, 1);
            
          if (i<4)
          {
               blcBlockToFrame(block, yBlockInFrame, param->yWidth);
               if (param->reversible_vlc && param->vlc_dec_direction) 
               {
                   yBlockInFrame -= 8;
                   if (i == 2)
                       yBlockInFrame -= ((/*8 **/ param->yWidth<<3) - 16);
               } 
               else 
               {
                   yBlockInFrame += 8;
                   if (i & 1)
                       yBlockInFrame += ((/*8 **/ param->yWidth<<3) - 16);
               }
          }
          else
          {
              blcBlockToFrame(block, ((i==4)? param->uBlockInFrame : param->vBlockInFrame) , cWidth);   
          }
          
      }
            
   }
   /* for blocks */
   
   if ( hTranscoder->TranscodingOneMB(NULL) != TX_OK )
   {
      return DMD_ERR;
   }
    
   return DMD_OK;
   
corruptedMB:

   return DMD_BIT_ERR;
}

/* {{-output"dmdGetAndDecodeMPEGPMBBlocks.txt"}} */

/* without the possibility to decode only Y component (#ifdef H263D_LUMINANCE_ONLY) */
/*
 * dmdGetAndDecodeMPEGPMBBlocks
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

int dmdGetAndDecodeMPEGPMBBlocks(
   dmdPParam_t *param, CMPEG4Transcoder *hTranscoder)
/* {{-output"dmdGetAndDecodeMPEGPMBBlocks.txt"}} */
{
   int i, j, 
      bitErrorIndication = 0, /* Carries bit error indication information returned
                                 by the video demultiplexer module */
      ret = 0;
      
   bibBuffer_t *inBuffer;

   TBool getDecodedFrame;

   inBuffer = param->inBuffer;
   getDecodedFrame = param->iGetDecodedFrame;

   int yWidth, block[64];
   u_char *yBlockInFrame;
   int chrQuant;

   int fEscapeCodeUsed = 0;

   /* Find out the value of QP for chrominance block. */
   chrQuant = param->quant;

   yWidth = param->uvWidth <<1 /** 2*/;
   if ( param->currYMBInFrame )
    {
        yBlockInFrame = param->currYMBInFrame + 
            ((param->reversible_vlc && param->vlc_dec_direction) ? ((/*8 **/ yWidth<<3) + 8) : 0);
    }
   else
    {
        yBlockInFrame = NULL;
    }

   /* Loop through the 4 Luminance and the 2 Chrominance blocks */
   for (j=0; j<=5; j++) {
         
         /* if reversible decoding, the block numbering is reverse */
         if (param->reversible_vlc && param->vlc_dec_direction) 
         {
            i = 5-j;
         }
         else 
         {
            i=j;
         }
         
     /* MVE */
         hTranscoder->BeginOneBlock(i);
         
         if (((i<4) && (vdxIsYCoded(param->cbpy, i + 1))) ||
             ((i==4) && (vdxIsUCoded(param->cbpc))) ||
             ((i==5) && (vdxIsVCoded(param->cbpc))))
         {
             
             /* Get DCT coefficients */
             if (param->reversible_vlc) 
             {
                if (!param->vlc_dec_direction)
                    ret = vdxGetRVLCDCTBlock(inBuffer, 0, 0, block,
                    &bitErrorIndication);
                else
                    ret = vdxGetRVLCDCTBlockBackwards(inBuffer, 0, 0, block,
                    &bitErrorIndication);
             } 
             else 
             {
                ret = vdxGetDCTBlock(inBuffer, 0, 1, block, 
                     &bitErrorIndication, 0, param->quant, &fEscapeCodeUsed);
             }
             if ( ret < 0 )
                return DMD_ERR;
             else if ( ret == VDX_OK_BUT_BIT_ERROR )
                return DMD_BIT_ERR;
             
            hTranscoder->AddOneBlockDataToMB(i, block);          
            if(getDecodedFrame || hTranscoder->NeedDecodedYUVFrame()) // we need the YUV frames.
            {
                 
                /* IDCT & dequant */
                dblIdctAndDequant(block, ((i<4)?param->quant:chrQuant), 0);

                if (i<4)
                {
                     
                    blcAddBlock(block, yBlockInFrame,
                        yWidth, 0, 0, 0);
                } 
                else 
                {
                    /* U or V component */

                    blcAddBlock(block, ((i==4)? param->currUBlkInFrame : param->currVBlkInFrame), param->uvWidth, 0, 0, 0);
                }
            }
             
        }
         
         /* MVE */
         else
         {
             /* this block is not coded */
             hTranscoder->AddOneBlockDataToMB(i, NULL);          
         }

         
         if ((i<4) && yBlockInFrame)
         {
             if (param->reversible_vlc && param->vlc_dec_direction) {
                 yBlockInFrame -= 8;
                 if (i == 2)
                     yBlockInFrame -= ((/*8 **/ yWidth<<3) - 16);
             } else {
                 yBlockInFrame += 8;
                 if (i & 1)
                     yBlockInFrame += ((/*8 **/ yWidth<<3) - 16);
             }
             
         }
         else   //u,v
         {
             /* nothing here */
         }
   }

     
     /* MVE */
   if ( hTranscoder->TranscodingOneMB(param) != TX_OK )
    {
    return DMD_ERR;
    }
   return DMD_OK;
}
// End of File
