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
* Advanced Intra Coding functions (MPEG-4).
*
*/




#include "h263dConfig.h"

#include "vdcaic.h"

#include "errcodes.h"
#include "debug.h"

#include "zigzag.h"

/*
 *
 * aicIntraDCSwitch
 *
 * Parameters:
 *    intraDCVLCThr  VOP-header field for QP dependent switching between 
 *                   MPEG-4 IntraDC coding and IntraAC VLC coding of the 
 *                   Intra DC coefficients
 *    QP             quantization parameter
 *
 * Function:
 *    This function decides based on the input parameters if the Intra DC
 *    should be decoded as MPEG-4 IntraDC (switched=0) or switched to  
 *    IntraAC VLC (switched=1)
 *
 * Returns:
 *    switched value
 *
 *
 *
 */

u_char aicIntraDCSwitch(int intraDCVLCThr, int QP) {

   if ( intraDCVLCThr == 0 ) {
      return 0;
   }
   else if ( intraDCVLCThr == 7 ) {
      return 1;
   }
   else if ( QP >= intraDCVLCThr*2+11 ) {
      return 1;
   }
   else {
      return 0;
   }
}

/*
 *
 * aicDCScaler
 *
 * Parameters:
 *    QP       quantization parameter
 *    type     type of Block "1" Luminance "2" Chrominance
 *
 * Function:
 *    Calculation of DC quantization scale according
 *    to the incoming QP and block type
 *
 * Returns:
 *    dcScaler value
 *
 *
 *
 */

int aicDCScaler (int QP, int type) {

   int dcScaler;
   if (type == 1) {
      if (QP > 0 && QP < 5) {
         dcScaler = 8;
      }
      else if (QP > 4 && QP < 9) {
         dcScaler = 2 * QP;
      }
      else if (QP > 8 && QP < 25) {
         dcScaler = QP + 8;
      }
      else {
         dcScaler = 2 * QP - 16;
      }
   }
   else {
      if (QP > 0 && QP < 5) {
         dcScaler = 8;
      }
      else if (QP > 4 && QP < 25) {
         dcScaler = (QP + 13) / 2;
      }
      else {
         dcScaler = QP - 6;
      }
   }
   return dcScaler;
}

/**
 * Small routine to compensate the QP value differences between the
 * predictor and current block AC coefficients
 *
 **/

static int compensateQPDiff(int val, int QP) {

   if (val<0) {
      return (val-(QP>>1))/QP;
   }
   else {
      return (val+(QP>>1))/QP;
   }
}

/**
 * Small routine to fill default prediction values into a dcStore entry
 *
 */

static void resetPredRow(int pred[])
{
   memset (pred, 0, 7 * sizeof(int));
}    

/*
 *
 * aicStart
 *
 * Parameters:
 *    aicData     aicData_t structure
 *    instance    pointer to vdcInstance_t structure
 *
 * Function:
 *    This function initialises dcStore and qpStore buffers.
 *    One should call aicStart in the beginning of each VOP.
 *
 * Returns:
 *    Nothing
 *
 * Error codes:
 *
 *
 */

void aicStart(aicData_t *aicData, int numMBsInMBLine, int16 *error)
{

   int i,j, numStoreUnits;
   int initXpos[6] = {-1, 0, -1, 0, -1, -1};
   int initYpos[6] = {-1, -1, 0, 0, -1, -1};
   int initXtab[6] = {1, 0, 3, 2, 4, 5};
   int initYtab[6] = {2, 3, 0, 1, 4, 5};
   int initZtab[6] = {3, 2, 1, 0, 4, 5};

   numStoreUnits = numMBsInMBLine*2;

   if (!aicData->qpStore || !aicData->dcStore) {
      aicData->qpStore = (int16 *) calloc(numStoreUnits, sizeof(int16));
      if (!aicData->qpStore)
      {
          *error = ERR_VDC_MEMORY_ALLOC;
          return;
      }
      
      /* allocate space for 3D matrix to keep track of prediction values
      for DC/AC prediction */
      
      aicData->dcStore = (int ***)calloc(numStoreUnits, sizeof(int **));
      if (!aicData->dcStore)
      {
          *error = ERR_VDC_MEMORY_ALLOC;
          return;
      }

      for (i = 0; i < numStoreUnits; i++)
      {
         aicData->dcStore[i] = (int **)calloc(6, sizeof(int *));
         if (!aicData->dcStore[i])
         {
             *error = ERR_VDC_MEMORY_ALLOC;
             return;
         }

         for (j = 0; j < 6; j++)
         {
            aicData->dcStore[i][j] = (int *)calloc(15, sizeof(int));
            if ( !(aicData->dcStore[i][j]) )
            {
                *error = ERR_VDC_MEMORY_ALLOC;
                return;
            }
         }
      }
      
      aicData->numMBsInMBLine = numMBsInMBLine;

      for (i= 0; i < 6; i++)
      {
         aicData->Xpos[i] = initXpos[i];
         aicData->Ypos[i] = initYpos[i];
         aicData->Xtab[i] = initXtab[i];
         aicData->Ytab[i] = initYtab[i];
         aicData->Ztab[i] = initZtab[i];
      }

      /* 1 << (instance->bits_per_pixel - 1) */
      aicData->midGrey = 1 << 7; 

      aicData->ACpred_flag = 1;
   } else {
      memset(aicData->qpStore, 0, numStoreUnits * sizeof(int16));
   }

}

/*
 * Clip the reconstructed coefficient when it is stored for prediction 
 *
 */

#define aicClip(rec) \
   ((rec < -2048) ? -2048 : ((rec > 2047) ? 2047 : rec))
   
/*
 *
 * aicBlockUpdate
 *
 * Parameters:
 *    aicData        aicData_t structure
 *    currMBNum      Current Macroblocks Number
 *
 * Function:
 *    This function fills up the dcStore and qpStore of current MB.
 *
 * Returns:
 *    Nothing
 *
 * Error codes:
 *    None
 *
 *
 */

void aicBlockUpdate (aicData_t *aicData, int currMBNum, int blockNum, int *block,
                int pquant, int DC_coeff)
{

   int n, currDCStoreIndex;
   assert(currMBNum >= 0);

   currDCStoreIndex = ((currMBNum / aicData->numMBsInMBLine) % 2) * aicData->numMBsInMBLine +
                  (currMBNum % aicData->numMBsInMBLine);

   if (block != NULL) {
      
      aicData->dcStore[currDCStoreIndex][blockNum][0] = aicClip(DC_coeff);
      block[0] = aicClip(block[0]);

      for (n = 1; n < 8; n++)
      {
         aicData->dcStore[currDCStoreIndex][blockNum][n] = block[zigzag[n]] = aicClip(block[zigzag[n]]);
         aicData->dcStore[currDCStoreIndex][blockNum][n+7] = block[zigzag[n<<3 /**8*/]] = aicClip(block[zigzag[n<<3 /**8*/]]);
      }
   }

   if (blockNum == 0) 
      aicData->qpStore[currDCStoreIndex]= (int16) pquant;
}

/*
 *
 * aicIsBlockValid
 *
 * Parameters:
 *    aicData        aicData_t structure
 *    currMBNum      Current Macroblocks Number
 *
 * Function:
 *    This function checks if the current MB has a valid entry in the 
 *    dcStore and qpStore (needed in predictor validation of I-MBs in a P-VOP).
 *
 * Returns:
 *    1 if MB has a valid entry
 *    0 if MB doesn't have a valid entry
 *
 * Error codes:
 *    None
 *
 *
 */

int aicIsBlockValid (aicData_t *aicData, int currMBNum)
{
   int currDCStoreIndex;
   assert(currMBNum >= 0);

   currDCStoreIndex = ((currMBNum / aicData->numMBsInMBLine) % 2) * aicData->numMBsInMBLine +
                  (currMBNum % aicData->numMBsInMBLine);

   if (aicData->qpStore[currDCStoreIndex] > 0 && aicData->qpStore[currDCStoreIndex] < 32)
      return 1;
   else
      return 0;
}

/*
 *
 * aicFree
 *
 * Parameters:
 *    aicData        aicData_t structure
 *    numOfMBs      Number of Macroblocks in VOP
 *
 * Function:
 *    This function frees the dynamic memory allocated by aicStart.
 *    aicFree should be called at least when exiting the main program.
 *    Alternatively it can be called whenever the playing a video has
 *    ended.
 *
 * Returns:
 *    Nothing
 *
 * Error codes:
 *    None
 *
 *
 */

void aicFree(aicData_t *aicData)
{
   int i,j;

   /* Free allocated memory for 3D matrix */
   if (aicData->dcStore) {
      for (i = 0; i < (aicData->numMBsInMBLine*2); i++)
      {
         for (j = 0; j < 6; j++)
            free((char *)aicData->dcStore[i][j]);
         free((char *)aicData->dcStore[i]);
      }
      free((char *)aicData->dcStore);
   }

   /* Free allocated memory for qpStore matrix */
   if (aicData->qpStore) {
      free((char *)aicData->qpStore);
   }
}

/*
 *
 * aicDCACrecon
 *
 * Parameters:
 *   aicData         aicData_t structure
 *   QP              QP of the current MB
 *   fTopMBMissing   flag indicating if the block above the current block
 *                   is outside of the current Video Packet or not a valid 
 *                   Intra block (in case of P-VOP decoding)
 *   fLeftMBMissing  flag indicating if the block left to the current block
 *                   is outside of the current Video Packet or not a valid 
 *                   Intra block (in case of P-VOP decoding)
 *   fBBlockOut      flag indicating if the top-left neighbour block of the
 *                   current block is outside of the current Video Packet
 *                   or not a valid Intra block (in case of P-VOP decoding)
 *   blockNum        number of the current block in the MB 
 *                   (0..3 luminance, 4..5 chrominance)
 *   qBlock          block of coefficients
 *   currMBNum       number of the current macroblocks in the VOP
 *
 * Function:
 *   This function reconstructs the DC and AC (first column or first row or none)
 *   coefficients of the current block by selecting the predictor from the 
 *   neighbouring blocks (or default values), and adding these predictor 
 *   values to qBlock. Its output is the quantized coefficient matrix in 
 *   normal zigzag order.
 *
 * Returns:
 *    Nothing
 *
 * Error codes:
 *    None
 *
 *
 */

void aicDCACrecon(aicData_t *aicData, int QP, u_char fTopMBMissing, u_char fLeftMBMissing,
              u_char fBBlockOut,  int blockNum, int *qBlock, int currMBNum)
{
   int m, n, tempDCScaler;
   int xCoordMB, yCoordMB, mbWidth, currDCStoreIndex;
   int blockA = 0, blockB = 0, blockC = 0, fBlockAExist = 0, fBlockCExist = 0;
   int gradHor, gradVer, predDC;
   int fVertical;
   int predA[7], predC[7];
   int pcoeff[64];
   assert(currMBNum >= 0);
   assert(qBlock != NULL);

   xCoordMB = currMBNum % aicData->numMBsInMBLine;
   yCoordMB = currMBNum / aicData->numMBsInMBLine;
   currDCStoreIndex = (yCoordMB % 2)*aicData->numMBsInMBLine + xCoordMB;
   mbWidth = aicData->numMBsInMBLine * ((currDCStoreIndex < aicData->numMBsInMBLine) ? -1 : 1);


   /* Find the direction of prediction and the DC prediction */
   switch ( blockNum ) {
   case 0 :
   case 4 :
   case 5 :
      /* Y0, U, and V blocks */


      /* Prediction blocks A (left), B (above-left), and C (above) */
      if ( ( yCoordMB == 0 && xCoordMB == 0) || (fTopMBMissing && fLeftMBMissing) || (xCoordMB == 0 && fTopMBMissing) || (yCoordMB == 0 && fLeftMBMissing) ) {
         /* top-left edge of VOP or VP */
         blockA = aicData->midGrey<<3 /**8*/;
         blockB = aicData->midGrey<<3 /**8*/;
         blockC = aicData->midGrey<<3 /**8*/;
         fBlockAExist = fBlockCExist = 0;
      }
      else if ( yCoordMB == 0 || fTopMBMissing ) {
         /* top row of VOP or VP, or MB on top not valid */
         blockA = aicData->dcStore[currDCStoreIndex+aicData->Xpos[blockNum]][aicData->Xtab[blockNum]][0];
         fBlockAExist = 1;
         if ( yCoordMB == 0 || fBBlockOut ) {
            /* B MB is out of this VP */
            blockB = aicData->midGrey<<3 /**8*/;
         }
         else {
            blockB = aicData->dcStore[currDCStoreIndex+(aicData->Ypos[blockNum])*mbWidth+aicData->Xpos[blockNum]][aicData->Ztab[blockNum]][0];
         }
         blockC = aicData->midGrey<<3 /**8*/;
         fBlockCExist = 0;
      }
      else if ( xCoordMB == 0 || fLeftMBMissing ) {
         /* left edge of VOP or VP, or MB on left not valid */
         blockA = aicData->midGrey<<3 /**8*/;
         fBlockAExist = 0;
         if ( xCoordMB == 0 || fBBlockOut ) {
            /* B MB is out of this VP */
            blockB = aicData->midGrey<<3 /**8*/;
         }
         else {
            blockB = aicData->dcStore[currDCStoreIndex+(aicData->Ypos[blockNum])*mbWidth+aicData->Xpos[blockNum]][aicData->Ztab[blockNum]][0];
         }
         blockC = aicData->dcStore[currDCStoreIndex+(aicData->Ypos[blockNum])*mbWidth][aicData->Ytab[blockNum]][0];
         fBlockCExist = 1;
      }
      else {
         /* Something else */
         blockA = aicData->dcStore[currDCStoreIndex+aicData->Xpos[blockNum]][aicData->Xtab[blockNum]][0];
         if ( fBBlockOut ) {
            /* B MB is out of this VP */
            blockB = aicData->midGrey<<3 /**8*/;
         }
         else {
            blockB = aicData->dcStore[currDCStoreIndex+(aicData->Ypos[blockNum])*mbWidth+aicData->Xpos[blockNum]][aicData->Ztab[blockNum]][0];
         }
         blockC = aicData->dcStore[currDCStoreIndex+(aicData->Ypos[blockNum])*mbWidth][aicData->Ytab[blockNum]][0];
         fBlockAExist = fBlockCExist = 1;
      }
      break;
   case 1 : 
      /* Y1 block */

      /* Prediction block A (left) always available */
      blockA = aicData->dcStore[currDCStoreIndex+aicData->Xpos[blockNum]][aicData->Xtab[blockNum]][0];
      fBlockAExist = 1;
      /* Prediction blocks B (above-left) and C (above) */
      if ( yCoordMB == 0 || fTopMBMissing ) {
         /* top row of VOP or VP */
         blockB = aicData->midGrey<<3 /**8*/;
         blockC = aicData->midGrey<<3 /**8*/;
         fBlockCExist = 0;
      }
      else {
         blockB = aicData->dcStore[currDCStoreIndex+(aicData->Ypos[blockNum])*mbWidth+aicData->Xpos[blockNum]][aicData->Ztab[blockNum]][0];
         blockC = aicData->dcStore[currDCStoreIndex+(aicData->Ypos[blockNum])*mbWidth][aicData->Ytab[blockNum]][0];
         fBlockCExist = 1;
      }
      break;
   case 2 :
      /* Y2 block */

      /* Prediction blocks A (left) and B (above-left) */
      if ( xCoordMB == 0 || fLeftMBMissing ) {
         /* left edge or first MB in VP */
         blockA = aicData->midGrey<<3 /**8*/;
         blockB = aicData->midGrey<<3 /**8*/;
         fBlockAExist = 0;
      }
      else {
         blockA = aicData->dcStore[currDCStoreIndex+aicData->Xpos[blockNum]][aicData->Xtab[blockNum]][0];
         blockB = aicData->dcStore[currDCStoreIndex+(aicData->Ypos[blockNum])*mbWidth+aicData->Xpos[blockNum]][aicData->Ztab[blockNum]][0];
         fBlockAExist = 1;
      }
      /* Prediction block C (above) always available */
      blockC = aicData->dcStore[currDCStoreIndex+(aicData->Ypos[blockNum])*mbWidth][aicData->Ytab[blockNum]][0];
      fBlockCExist = 1;
      break;
   case 3 :
      /* Y3 block */

      /* Prediction block A (left) always available */
      blockA = aicData->dcStore[currDCStoreIndex+aicData->Xpos[blockNum]][aicData->Xtab[blockNum]][0];
      /* Prediction block B (above-left) always available */
      blockB = aicData->dcStore[currDCStoreIndex+(aicData->Ypos[blockNum])*mbWidth+aicData->Xpos[blockNum]][aicData->Ztab[blockNum]][0];
      /* Prediction block C (above) always available */
      blockC = aicData->dcStore[currDCStoreIndex+(aicData->Ypos[blockNum])*mbWidth][aicData->Ytab[blockNum]][0];
      fBlockAExist = fBlockCExist = 1;
      break;
   }




   gradHor = blockB - blockC;
   gradVer = blockA - blockB;

   if ((abs(gradVer)) < (abs(gradHor))) {
      /* Vertical prediction (from C) */
      predDC = blockC;
      fVertical = 1;
   }
   else {
      /* Horizontal prediction (from A) */
      predDC = blockA;
      fVertical = 0;
   }

   /* Now reconstruct the DC coefficient */
   tempDCScaler = aicDCScaler(QP,(blockNum<4)?1:2);
   qBlock[0] = (u_int8) (qBlock[0] + (predDC + tempDCScaler/2) / tempDCScaler);

   /* Do AC prediction if required */
   if (aicData->ACpred_flag == 1) {

      /* Do inverse zigzag-scanning */
      if (fVertical) {
         for (m = 0; m < 64; m++) {
            pcoeff[m] = qBlock[zigzag_h[m]];
         }
      }
      else { /* horizontal prediction */
         for (m = 0; m < 64; m++) {
            pcoeff[m] = qBlock[zigzag_v[m]];
         }
      }
   


      /* AC predictions */
      if ( !fVertical && fBlockAExist ) {
         /* prediction from A */
         for (m = 8; m < 15; m++) {
            predA[m-8] = compensateQPDiff(((aicData->dcStore[currDCStoreIndex+aicData->Xpos[blockNum]][aicData->Xtab[blockNum]][m]) * 2 * (aicData->qpStore[currDCStoreIndex+aicData->Xpos[blockNum]])), (QP<<1));
         }
      }
      else if ( fVertical && fBlockCExist ) {
         /* prediction from C */
         for (m = 1; m < 8; m++) {
            predC[m-1] = compensateQPDiff(((aicData->dcStore[currDCStoreIndex+(aicData->Ypos[blockNum])*mbWidth][aicData->Ytab[blockNum]][m]) * 2 * (aicData->qpStore[currDCStoreIndex+(aicData->Ypos[blockNum])*mbWidth])), (QP<<1));
         }
      }
      else {
         /* Prediction not possible */
         if ( fVertical ) {
            resetPredRow(predC);
         }
         else {
            resetPredRow(predA);
         }
      }



      /* AC coefficients reconstruction*/
      if (fVertical) { /* Vertical, top row of block C */
         for (m = 0; m < 7; m++) {
            qBlock[zigzag[(m+1)*8]] = pcoeff[(m+1)*8];
         }
         for (m = 1; m < 8; m++) {
            qBlock[zigzag[m]] = pcoeff[m] + predC[m-1];
         }
      }
      else { /* Horizontal, left column of block A */
         for (m = 0; m < 7; m++) {
            qBlock[zigzag[(m+1)*8]] = pcoeff[(m+1)*8] + predA[m];
         }
         for (m = 1; m < 8; m++) {
            qBlock[zigzag[m]] = pcoeff[m];
         }
      }

      /* Copy the rest of the coefficients back to qBlock */
      for (m = 1; m < 8; m++) {
         for (n = 1; n < 8; n++) {
            qBlock[zigzag[m*8+n]] = pcoeff[m*8+n];
         }
      }
   }

}

// End of file
