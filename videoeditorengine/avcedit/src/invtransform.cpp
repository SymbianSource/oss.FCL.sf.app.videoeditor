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
*
*/


#include "globals.h"
#include "invtransform.h"


#ifndef AVC_RECO_BLOCK_ASM

/*
 *
 * itrIDCTdequant4x4:
 *
 * Parameters:
 *      src                   Source values
 *      dest                  Inverse transformed values
 *      dequantPtr            Dequant coefficients
 *      qp_per                qp/6
 *      isDc                  True if DC is separate
 *      dcValue               Possible DC value
 *
 * Function:
 *      Dequantize coefficients and compute approximate 4x4 inverse DCT.
 *
 * Returns:
 *      -
 */
void itrIDCTdequant4x4(int src[4][4], int dest[4][4], const int *dequantPtr,
                       int qp_per, int isDc, int dcValue)
{
  int tmp[4][4];
  int A, B, C, D, E, F;
  int i;
  int deqc;

  /*
   *  a = A + B + C + (D>>1)
   *  b = A + (B>>1) - C - D
   *  c = A - (B>>1) - C + D
   *  d = A - B + C - (D>>1)
   *   =>
   *  E = A + C
   *  F = B +  (D>>1)
   *  a = E + F
   *  d = E - F
   *  E = A - C
   *  F = (B>>1) - D
   *  b = E + F
   *  c = E - F
   */

  A = dcValue;

  for (i = 0; i < 4; i++) {
    deqc = (*dequantPtr++) << qp_per;

    if (!isDc)
      A = src[i][0] * deqc;

    C = src[i][2] * deqc;

    deqc = (*dequantPtr++) << qp_per;

    B = src[i][1] * deqc;
    D = src[i][3] * deqc;

    E = A +  C;
    F = B + (D>>1);
    tmp[i][0] = E + F;
    tmp[i][3] = E - F;
    E =  A     - C;
    F = (B>>1) - D;
    tmp[i][1] = E + F;
    tmp[i][2] = E - F;

    isDc = 0;
  }

  for (i = 0; i < 4; i++) {
    E = tmp[0][i] +  tmp[2][i];
    F = tmp[1][i] + (tmp[3][i]>>1);
    dest[0][i] = E + F;
    dest[3][i] = E - F;
    E =  tmp[0][i]     - tmp[2][i];
    F = (tmp[1][i]>>1) - tmp[3][i];
    dest[1][i] = E + F;
    dest[2][i] = E - F;
  }
}

#endif


/*
 *
 * itrIHadaDequant4x4:
 *
 * Parameters:
 *      src                   Source values
 *      dest                  Inverse transformed values
 *      deqc                  Dequantization coefficient
 *
 * Function:
 *      Compute 4x4 inverse Hadamard transform and dequantize coefficients.
 *
 * Returns:
 *      -
 *
 */
void itrIHadaDequant4x4(int src[4][4], int dest[4][4], int deqc)
{
  int tmp[4][4];
  int E;
  int F;
  int i;

  for (i = 0; i < 4; i++) {
    E = src[i][0] + src[i][2];
    F = src[i][1] + src[i][3];
    tmp[i][0] = E + F;
    tmp[i][3] = E - F;
    E = src[i][0] - src[i][2];
    F = src[i][1] - src[i][3];
    tmp[i][1] = E + F;
    tmp[i][2] = E - F;
  }

  for (i = 0; i < 4; i++) {
    E = tmp[0][i] + tmp[2][i];
    F = tmp[1][i] + tmp[3][i];
    dest[0][i] = ((E + F) * deqc + 2) >> 2;
    dest[3][i] = ((E - F) * deqc + 2) >> 2;
    E = tmp[0][i] - tmp[2][i];
    F = tmp[1][i] - tmp[3][i];
    dest[1][i] = ((E + F) * deqc + 2) >> 2;
    dest[2][i] = ((E - F) * deqc + 2) >> 2;
  }
}


/*
 *
 * itrIDCTdequant2x2:
 *
 * Parameters:
 *      src                   Source values
 *      dest                  Inverse transformed values
 *      deqc                  Dequantization coefficient
 *
 * Function:
 *      Compute 2x2 inverse DCT and dequantize coefficients.
 *
 * Returns:
 *      -
 *
 */
void itrIDCTdequant2x2(int src[2][2], int dest[2][2], int deqc)
{
  int DDC00 = src[0][0];
  int DDC10 = src[0][1];
  int DDC01 = src[1][0];
  int DDC11 = src[1][1];
  int A, B;

  /*
   *  DDC(0,0) DDC(1,0)  =>  DC0 DC1
   *  DDC(0,1) DDC(1,1)      DC2 DC3
   *
   *  DC0 = (DDC00+DDC10+DDC01+DDC11)
   *  DC1 = (DDC00-DDC10+DDC01-DDC11)
   *  DC2 = (DDC00+DDC10-DDC01-DDC11)
   *  DC3 = (DDC00-DDC10-DDC01+DDC11)
   */

  A = DDC00 + DDC01;
  B = DDC10 + DDC11;
  dest[0][0] = ((A + B) * deqc) >> 1;
  dest[0][1] = ((A - B) * deqc) >> 1;
  A = DDC00 - DDC01;
  B = DDC10 - DDC11;
  dest[1][0] = ((A + B) * deqc) >> 1;
  dest[1][1] = ((A - B) * deqc) >> 1;
}
