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
* Multiple scan-order macroblock decoding functions.
*
*/


/*
 * Includes
 */
#include "h263dConfig.h"
#include "decmbs.h"
#include "viddemux.h"
/* MVE */
#include "MPEG4Transcoder.h"


/*
 * Global functions
 */

/* {{-output"dmbsGetAndDecodeIMBsInScanOrder.txt"}} */
/*
 * dmbsGetAndDecodeIMBsInScanOrder
 *
 * Parameters:
 *    numMBsToDecode             the number of macroblocks to decode
 *    inParam                    input parameters
 *    inOutParam                 input/output parameters, these parameters
 *                               may be modified in the function
 *    quant                      array for storing quantization parameters
 *
 * Function:
 *    This function gets and decodes a requested number of INTRA frame
 *    macroblocks in scan order.
 *
 * Returns:
 *    >= 0                       the function was successful
 *    < 0                        an error occured
 *
 */

int dmbsGetAndDecodeIMBsInScanOrder(
   const dmbIFrameMBInParam_t *inParam,
   dmbIFrameMBInOutParam_t *inOutParam,
   int *quant, CMPEG4Transcoder *hTranscoder)
/* {{-output"dmbsGetAndDecodeIMBsInScanOrder.txt"}} */
{
   int currMBNum;
   int yWidth = inParam->pictParam->lumMemWidth;
   int uvWidth = yWidth / 2;
   int ret = 0;
   dmbIFrameMBInParam_t dmbi;

   memcpy(&dmbi, inParam, sizeof(dmbIFrameMBInParam_t));

   for (currMBNum = 0; currMBNum < inParam->numMBsInSegment; currMBNum++) {

      hTranscoder->BeginOneMB(currMBNum + inParam->numMBsInSegment * inParam->yPosInMBs);

      ret = dmbGetAndDecodeIFrameMB(&dmbi, inOutParam, 0, hTranscoder);
      if ( ret < 0)
         return DMBS_ERR;
      else if ( ret == DMB_BIT_ERR ) {
         break;
      }

      /* Store quantizer and increment array index */
      *quant = inOutParam->quant;
      quant++;

      if ( inOutParam->yMBInFrame != NULL )
        {
          inOutParam->yMBInFrame += 16;
          inOutParam->uBlockInFrame += 8;
          inOutParam->vBlockInFrame += 8;
        }
      dmbi.xPosInMBs++;

      if (dmbi.xPosInMBs == inParam->pictParam->numMBsInMBLine) {
          if ( inOutParam->yMBInFrame != NULL )
            {
                inOutParam->yMBInFrame += 15 * yWidth;
                inOutParam->uBlockInFrame += 7 * uvWidth;
                inOutParam->vBlockInFrame += 7 * uvWidth;
            }
         dmbi.xPosInMBs = 0;
         dmbi.yPosInMBs++;
      }
   }
     
   hTranscoder->H263OneGOBSliceEnded(dmbi.yPosInMBs * inParam->numMBsInSegment);
   
   return DMBS_OK;
}


/* {{-output"dmbsGetAndDecodePMBsInScanOrder.txt"}} */
/*
 * dmbsGetAndDecodePMBsInScanOrder
 *
 * Parameters:
 *    numMBsToDecode             the number of macroblocks to decode
 *    inParam                    input parameters
 *    inOutParam                 input/output parameters, these parameters
 *                               may be modified in the function
 *    quant                      array for storing quantization parameters
 *
 * Function:
 *    This function gets and decodes a requested number of INTER frame
 *    macroblocks in scan order.
 *
 * Returns:
 *    >= 0                       the function was successful
 *    < 0                        an error occured
 *
 */

int dmbsGetAndDecodePMBsInScanOrder(
   const dmbPFrameMBInParam_t *inParam,
   dmbPFrameMBInOutParam_t *inOutParam,
   int *quant, CMPEG4Transcoder *hTranscoder)
/* {{-output"dmbsGetAndDecodePMBsInScanOrder.txt"}} */
{
   int currMBNum;
   int yWidth = inParam->pictParam->lumMemWidth;
   int uvWidth = yWidth / 2;
   int ret = 0;
   dmbPFrameMBInParam_t dmbi;

   memcpy(&dmbi, inParam, sizeof(dmbPFrameMBInParam_t));

   for (currMBNum = 0; currMBNum < inParam->numMBsInSegment; currMBNum++) {
         
      hTranscoder->BeginOneMB(currMBNum + inParam->numMBsInSegment * inParam->yPosInMBs);

      ret = dmbGetAndDecodePFrameMB(&dmbi, inOutParam, 0, hTranscoder);
      if (ret < 0)
        return DMBS_ERR;
      else if ( ret == DMB_BIT_ERR ) {
        break;
      }
         
      /* Store quantizer and increment array index */
      *quant = inOutParam->quant;
      quant++;
      
      if ( inOutParam->yMBInFrame != NULL )
        {
          inOutParam->yMBInFrame += 16;
          inOutParam->uBlockInFrame += 8;
          inOutParam->vBlockInFrame += 8;
        }
      dmbi.xPosInMBs++;

      if (dmbi.xPosInMBs == inParam->pictParam->numMBsInMBLine) {
          if ( inOutParam->yMBInFrame != NULL )
            {
             inOutParam->yMBInFrame += 15 * yWidth;
             inOutParam->uBlockInFrame += 7 * uvWidth;
             inOutParam->vBlockInFrame += 7 * uvWidth;
            }
         dmbi.xPosInMBs = 0;
         dmbi.yPosInMBs++;
      }
   }
     
   hTranscoder->H263OneGOBSliceEnded(dmbi.yPosInMBs * inParam->numMBsInSegment);
   
   return DMBS_OK;
     
}
// End of File
