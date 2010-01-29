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
* Header for the aic (Advanced Intra Coding) module.
*
*/



#ifndef _VDCAIC_H_
#define _VDCAIC_H_

#include "epoclib.h"

/*
 * Structs and typedefs
 */

typedef struct {
   
    int16 *qpStore;    /* storage place for the QP values of MBs in the 
                           previous and current MB line */

    int ***dcStore;    /* storage place for the DC and first horizontal 
                           and vertical line of AC coefficients of the 
                           4+1+1 blocks of the MBs in the previous and 
                           current MB line */

    int numMBsInMBLine; /* number of MBs in a MB line for the calculation 
                           of the internal sorage index from currMBNum */

    int Xpos[6];        /* indexed by block number (0..5) of an MB gives 
                           relative MB index of left neighbor block */

    int Ypos[6];        /* indexed by block number (0..5) of an MB gives 
                           relative MB index of top neighbor block */

    int Xtab[6];        /* indexed by block number (0..5) of an MB gives 
                           block number of left neighbor block */

    int Ytab[6];        /* indexed by block number (0..5) of an MB gives 
                           block number of top neighbor block */

    int Ztab[6];        /* indexed by block number (0..5) of an MB gives 
                           block number of top-left neighbor block */

    u_char ACpred_flag; /* ON(1): AC prediction is used, OFF(0): only DC */

    int midGrey;       /* the grey value used for prediction direction
                           decision if prediction block is not available */
} aicData_t;


/*
 * Prototypes
 */

   void aicStart (aicData_t *aicData, int numMBsInMBLine, int16 *error);

   void aicFree (aicData_t *aicData);

   void aicBlockUpdate (aicData_t *aicData, int currMBNum, int blockNum, int *block,
                        int pquant, int DC_coeff);

   int aicIsBlockValid (aicData_t *aicData, int currMBNum);

   void aicDCACrecon (aicData_t *aicData, int QP, u_char fTopMBMissing, u_char fLeftMBMissing,
                      u_char fBBlockOut, int blockNum, int *qBlock, int currMBNum);

   int aicDCScaler (int QP, int type);

   u_char aicIntraDCSwitch (int intraDCVLCThr, int QP);

#endif
// End of File
