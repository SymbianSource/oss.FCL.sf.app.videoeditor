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
* Block decoding functions.
*
*/



#include "h263dConfig.h"

#include "decblock.h"
#include "blkfunc.h"
#include "viddemux.h"
#include "idct.h"   


/* {{-output"dblfree.txt"}} */
/*
 * dblFree
 *    
 *
 * Parameters:
 *    None.
 *
 * Function:
 *    This function deinitializes the module.
 *    Any functions of this module must not be called after dblFree (except 
 *    for dblLoad).
 *
 * Returns:
 *    >= 0  if succeeded
 *    < 0   if failed
 *
 */

int dblFree(void)
/* {{-output"dblfree.txt"}} */
{
   return 0;
}


/* {{-output"dblload.txt"}} */
/*
 * dblLoad
 *    
 *
 * Parameters:
 *    None.
 *
 * Function:
 *    This function initializes the module.
 *    dblLoad has to be called before any other function of this module
 *    is used.
 *
 * Returns:
 *    >= 0  if succeeded
 *    < 0   if failed
 *
 */

int dblLoad(void)
/* {{-output"dblload.txt"}} */
{    
   return 0;
}


/* {{-output"dblIdctAndDequant.txt"}} */
/*
 * dblIdctAndDequant
 *
 * Parameters:
 *    block          block array (length 64)
 *    quant          quantization information
 *    skip           must be 1 if INTRADC is in the block, otherwise 0
 *
 * Function:
 *    This function makes the dequantization, the clipping and the inverse
 *    cosine transform for the given block.
 *
 * Returns:
 *    Nothing.
 *
 * Error codes:
 *    None.
 *
 */

void dblIdctAndDequant(int *block, int quant, int skip)
/* {{-output"dblIdctAndDequant.txt"}} */
{

   int rec, i, *tmpBlock, level;

   /* See section 6.2.1 of H.263 Recommendation for Inverse Quantization
      formulas. */

   /* If odd quantization parameter */
   if (quant & 1) {

      for (i = 64 - skip, tmpBlock = block + skip; i; i--, tmpBlock++) {

         if (!(*tmpBlock))
            continue;

         level = *tmpBlock;

         if (level > 0) {
            rec = quant * ((level << 1) + 1);
            *tmpBlock = (rec < 2048) ? rec : 2047;
         }

         else {
            rec = -(quant * (((-level) << 1) + 1));
            *tmpBlock = (rec < -2048) ? -2048 : rec;
         }
      }
   }

   /* Else even quantization parameter */
   else {

      /* For loop copy-pasted from the previous case due to speed 
         optimization */
      for (i = 64 - skip, tmpBlock = block + skip; i; i--, tmpBlock++) {

         if (!(*tmpBlock))
            continue;

         level = *tmpBlock;

         if (level > 0) {
            rec = quant * ((level << 1) + 1) - 1;
            *tmpBlock = (rec < 2048) ? rec : 2047;
         }

         else {
            rec = -(quant * (((-level) << 1) + 1) - 1);
            *tmpBlock = (rec < -2048) ? -2048 : rec;
         }
      }
   }
   idct(block);
}


/*
 * dblIdct
 *
 * Parameters:
 *    block          block array (length 64)
 *
 * Function:
 *    This function makes the inverse
 *    cosine transform for the given block.
 *
 * Returns:
 *    Nothing.
 *
 * Error codes:
 *    None.
 *
 */

void dblIdct(int *block)

{
   idct(block);
}
// End of File
