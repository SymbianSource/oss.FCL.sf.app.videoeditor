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
* Implementation for MPEG4(H263) video transcoder. 
* VLC encoding for RVLC -> VLC  AC/DCT coeficients in the transcoder.
*
*/


#include "biblin.h"
#include "common.h"


/* Run-length coding */
typedef struct{
    int16 run;
    int level;
    int16 last;
} tEvent;

/* Variable length coding */
typedef struct{
    u_int16 code;
    u_int16 length;
} tVLCTable;


/* 
 * Function Declarations 
 */
int32 vlbPutACCoeffSVH(int32 coeffStart, int16* block, bibBuffer_t * outBuf,
                       int32 svh, int32 lastPos);
int32 vlbPutACVLCCoeff(bibBuffer_t *outBuf, tEvent *event, int32 svh);
int32 vlbPutDCCoeffCMT(int val, bibBuffer_t * outBuf, int32  blkCnt);
int32 vlbPutIntraACCoeffCMTDP(bibBuffer_t *outBuf, tEvent *event);
int32 vlbPutIntraACCoeffCMT(int32 coeffStart, int* block, bibBuffer_t * outBuf);
void  vdtPutBits (void *outBuf,  int numBits, unsigned int value);


/* 
 * Function Definitions 
 */


/* {{-output"vlbPutBits.txt"}} */
/*
* vlbPutBits
*
* Parameters:
*     outBuf            output buffer
*           numBits         number of bits to write
*           value             value to write
*
* Function:
*     This function puts some bits to the output buffer
*
* Returns:
*     Nothing.
*
* Error codes:
*     None.
*
*/
void vlbPutBits (bibBuffer_t *outBuf,
                 int32        numBits,
                 u_int32      value)
{
    // To be compatible with the old codes, I don't change the interface
    vdtPutBits((void *)outBuf, numBits, value);
}


/* 
 * Look-up tables 
 */

/* INTRA VLC tables */

/* run == 0 && level < 28 */
const tVLCTable vlbVLCTable1IntraAC[28] =
{
    {NOT_VALID,NOT_VALID},    {0x04,0x03}, {0x0c,0x04}, 
        {0x1e,0x05}, {0x1a,0x06}, {0x18,0x06}, {0x2a,0x07}, 
        {0x26,0x07}, {0x24,0x07}, {0x2e,0x08}, {0x3e,0x09}, 
        {0x3c,0x09}, {0x3a,0x09}, {0x4a,0x0a}, {0x48,0x0a},
        {0x46,0x0a}, {0x42,0x0a}, {0x42,0x0b}, {0x40,0x0b},
        {0x1e,0x0b}, {0x1c,0x0b}, {0x0e,0x0c}, {0x0c,0x0c},
        {0x40,0x0c}, {0x42,0x0c}, {0xa0,0x0d}, {0xa2,0x0d},
        {0xa4,0x0d}
};

/* run == 1 && level < 11 */
const tVLCTable vlbVLCTable2IntraAC[11] =
{
    {NOT_VALID, NOT_VALID},   {0x1c,0x05}, {0x28,0x07}, 
        {0x2c,0x08}, {0x38,0x09}, {0x40,0x0a}, {0x3e,0x0a}, 
        {0x1a,0x0b}, {0x44,0x0c}, {0xa6,0x0d}, {0xaa,0x0d}
};

/* run == 2 && level < 6 */
const tVLCTable vlbVLCTable3IntraAC[6] =
{
    {NOT_VALID, NOT_VALID},   {0x16,0x06}, {0x2a,0x08}, 
        {0x3c,0x0a}, {0x18,0x0b}, {0xac,0x0d}
};

/* run == 3 && level < 5 */
const tVLCTable vlbVLCTable4IntraAC[5] = 
{
    {NOT_VALID, NOT_VALID},   {0x22,0x07}, {0x36,0x09}, 
        {0x3a,0x0a}, {0x16,0x0b}
};

/* 4 <= run < 8 && level < 4 */
const tVLCTable vlbVLCTable5IntraAC[4][4] =
{
  {
        {NOT_VALID, NOT_VALID},   {0x20,0x07}, {0x44,0x0a}, 
        {0x14,0x0b}
  },
  {
    {NOT_VALID, NOT_VALID},   {0x1a,0x07}, {0x38,0x0a}, 
        {0x10,0x0b}
  },
  {
    {NOT_VALID, NOT_VALID},   {0x24,0x08}, {0x36,0x0a}, 
        {0xa8,0x0d}
  },
  {
    {NOT_VALID, NOT_VALID},   {0x28,0x08}, {0x34,0x0a}, 
        {0xae,0x0d}
  }
};

/* 8 <= run < 10 && level < 3 */
const tVLCTable vlbVLCTable6IntraAC[2][3] =
{
  {
        {NOT_VALID, NOT_VALID},   {0x32,0x09}, {0x12,0x0b}
  },
  {
    {NOT_VALID, NOT_VALID},   {0x30,0x09}, {0x46,0x0c}
  }
};

/* 10 <= run < 15 && level < 2 */
const tVLCTable vlbVLCTable7IntraAC[5][2] = 
{
  {
    {NOT_VALID, NOT_VALID},   {0x2e,0x09}
  },
  {
    {NOT_VALID, NOT_VALID},   {0x32,0x0a}
  },
  {
    {NOT_VALID, NOT_VALID},   {0x30,0x0a}
  },
  {
    {NOT_VALID, NOT_VALID},   {0x0e,0x0b}
  },
  {
    {NOT_VALID, NOT_VALID},   {0xb0,0x0d}
  }
};

/* run == 0 && level < 9 */
const tVLCTable vlbVLCTable8IntraAC[9] = 
{
    {NOT_VALID, NOT_VALID},   {0x0e,0x05}, {0x18,0x07}, 
        {0x2c,0x09}, {0x2e,0x0a}, {0x0c,0x0b}, {0x0a,0x0c},
        {0x08,0x0c}, {0xb2,0x0d}
};

/* run == 1 && level < 4 */
const tVLCTable vlbVLCTable9IntraAC[4] =
{
    {NOT_VALID, NOT_VALID},   {0x1e,0x07}, {0x2c,0x0a}, 
        {0x0a,0x0b}
};

/* 2 <= run < 7 && level < 3 */
const tVLCTable vlbVLCTable10IntraAC[5][3] =
{
  {
    {NOT_VALID, NOT_VALID},   {0x1c,0x07}, {0x08,0x0b}
  },
  {
    {NOT_VALID, NOT_VALID},   {0x22,0x08}, {0x48,0x0c}
  },
  {
    {NOT_VALID, NOT_VALID},   {0x20,0x08}, {0x4a,0x0c}
  },
  {
    {NOT_VALID, NOT_VALID},   {0x26,0x08}, {0xb4,0x0d}
  },
  {
    {NOT_VALID, NOT_VALID},   {0x2a,0x09}, {0xb6,0x0d}
  }
};

/* 7 <= run < 21 && level == 1 */
const tVLCTable vlbVLCTable11IntraAC[14][2] =
{
  {
    {NOT_VALID, NOT_VALID},   {0x28,0x09}
  },
  {
    {NOT_VALID, NOT_VALID},   {0x26,0x09}
  },
  {
    {NOT_VALID, NOT_VALID},   {0x34,0x09}
  },
  {
    {NOT_VALID, NOT_VALID},   {0x2a,0x0a}
  },
  {
    {NOT_VALID, NOT_VALID},   {0x28,0x0a}
  },
  {
    {NOT_VALID, NOT_VALID},   {0x26,0x0a}
  },
  {
    {NOT_VALID, NOT_VALID},   {0x24,0x0a}
  },
  {
    {NOT_VALID, NOT_VALID},   {0x22,0x0a}
  },
  {
    {NOT_VALID, NOT_VALID},   {0x4c,0x0c}
  },
  {
    {NOT_VALID, NOT_VALID},   {0x4e,0x0c}
  },
  {
    {NOT_VALID, NOT_VALID},   {0xb8,0x0d}
  },
  {
    {NOT_VALID, NOT_VALID},   {0xba,0x0d}
  },
  {
    {NOT_VALID, NOT_VALID},   {0xbc,0x0d}
  },
  { 
    {NOT_VALID, NOT_VALID},   {0xbe,0x0d}
  }
};


/* Inter AC VLC tables */

const tVLCTable vlbVLCTable1InterAC[13] =
{
    {NOT_VALID, NOT_VALID},
    {0x04,0x03}, {0x1e,0x05}, {0x2a,0x07}, {0x2e,0x08},
    {0x3e,0x09}, {0x4a,0x0a}, {0x48,0x0a}, {0x42,0x0b},
    {0x40,0x0b}, {0x0e,0x0c}, {0x0c,0x0c}, {0x40,0x0c}
};

const tVLCTable vlbVLCTable2InterAC[7] =
{
    {NOT_VALID, NOT_VALID},   {0x0c,0x04}, {0x28,0x07}, 
        {0x3c,0x09}, {0x1e,0x0b}, {0x42,0x0c}, {0xa0,0x0d}
};

const tVLCTable vlbVLCTable3InterAC[5] =
{
    {NOT_VALID, NOT_VALID},   {0x1c,0x05}, {0x3a,0x09}, 
        {0x1c,0x0b}, {0xa2,0x0d}
};

const tVLCTable vlbVLCTable4InterAC[4][4] = 
{
  {
    {NOT_VALID, NOT_VALID},   {0x1a,0x06}, {0x46,0x0a}, 
        {0x1a,0x0b}
  },
  {
    {NOT_VALID, NOT_VALID},   {0x18,0x06}, {0x44,0x0a}, 
        {0xa4,0x0d}
  },
  {
    {NOT_VALID, NOT_VALID},   {0x16,0x06}, {0x18,0x0b}, 
        {0xa6,0x0d}
  },
  {
    {NOT_VALID, NOT_VALID},   {0x26,0x07}, {0x16,0x0b}, 
        {0xa8,0x0d}
  }
};

const tVLCTable vlbVLCTable5InterAC[4][3] =
{
  {
    {NOT_VALID, NOT_VALID},   {0x24,0x07}, {0x14,0x0b}
  },
  {
    {NOT_VALID, NOT_VALID},   {0x22,0x07}, {0x12,0x0b}
  },
  {
    {NOT_VALID, NOT_VALID},   {0x20,0x07}, {0x10,0x0b}
  },
  {
    {NOT_VALID, NOT_VALID},   {0x2c,0x08}, {0xaa,0x0d}
  }
};

const tVLCTable vlbVLCTable6InterAC[16] =
{
    {0x2a,0x08}, {0x28,0x08}, {0x38,0x09}, {0x36,0x09},
    {0x42,0x0a}, {0x40,0x0a}, {0x3e,0x0a}, {0x3c,0x0a}, 
        {0x3a,0x0a}, {0x38,0x0a}, {0x36,0x0a}, {0x34,0x0a}, 
        {0x44,0x0c}, {0x46,0x0c}, {0xac,0x0d}, {0xae,0x0d}
};

const tVLCTable vlbVLCTable7InterAc[4] =
{
    {NOT_VALID, NOT_VALID},   {0x0e,0x05}, {0x32,0x0a}, 
        {0x0a,0x0c}
};

const tVLCTable vlbVLCTable8InterAC[3] =
{
    {NOT_VALID, NOT_VALID},   {0x1e,0x07}, {0x08,0x0c}
};

const tVLCTable vlbVLCTable9InterAC[39] =
{
    {0x1c,0x07}, {0x1a,0x07}, {0x18,0x07}, {0x26,0x08},
    {0x24,0x08}, {0x22,0x08}, {0x20,0x08}, {0x34,0x09},
    {0x32,0x09}, {0x30,0x09}, {0x2e,0x09}, {0x2c,0x09},
    {0x2a,0x09}, {0x28,0x09}, {0x26,0x09}, {0x30,0x0a},
    {0x2e,0x0a}, {0x2c,0x0a}, {0x2a,0x0a}, {0x28,0x0a},
    {0x26,0x0a}, {0x24,0x0a}, {0x22,0x0a}, {0x0e,0x0b},
    {0x0c,0x0b}, {0x0a,0x0b}, {0x08,0x0b}, {0x48,0x0c},
    {0x4a,0x0c}, {0x4c,0x0c}, {0x4e,0x0c}, {0xb0,0x0d}, 
        {0xb2,0x0d}, {0xb4,0x0d}, {0xb6,0x0d}, {0xb8,0x0d}, 
        {0xba,0x0d}, {0xbc,0x0d}, {0xbe,0x0d}
};


const int8 vlbVLCRun0InterAC[64] =
{
    0x0d, 0x07, 0x05, 0x04, 0x04, 0x04, 0x04, 0x03, 
      0x03, 0x03, 0x03, 0x02, 0x02, 0x02, 0x02, 0x02, 
      0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 
      0x02, 0x02, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

const int8 vlbVLCRun1InterAC[64] =
{
      0x04, 0x03, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 
      0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 
      0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 
      0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 
      0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 
      0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 
};

/* other tables */
const int8 vlbLMaxTableIntra[36] =
{
    0x1b, 0x0a, 0x05, 0x04, 0x03, 0x03, 
        0x03, 0x03, 0x02, 0x02, 0x01, 0x01, 
        0x01, 0x01, 0x01, 0x08, 0x03, 0x02, 
        0x02, 0x02, 0x02, 0x02, 0x01, 0x01, 
        0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 
        0x01, 0x01, 0x01, 0x01, 0x01, 0x01
};

const int8 vlbLMaxTableInter[68] =
{
    0x0c, 0x06, 0x04, 0x03, 0x03, 0x03, 
        0x03, 0x02, 0x02, 0x02, 0x02, 0x01, 
        0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 
        0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 
        0x01, 0x01, 0x01, 0x03, 0x02, 0x01, 
        0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 
        0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 
        0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 
        0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 
        0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 
        0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 
        0x01, 0x01
};

const int8 vlbRMaxTableIntra[35] =
{
    0x0f, 0x0a, 0x08, 0x04, 0x03, 0x02, 
        0x02, 0x02, 0x02, 0x02, 0x01, 0x01, 
        0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 
        0x01, 0x01, 0x01, 0x15, 0x07, 0x02,
    0x01, 0x01, 0x01, 0x01, 0x01
};

const int8 vlbRMaxTableInter[15] =
{
    0x1b, 0x0b, 0x07, 0x03, 0x02, 0x02,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x29, 0x02, 0x01
};


const u_int32 vlbZigzagScan[64] = 
{
    0x00, 0x01, 0x08, 0x10, 0x09, 0x02, 0x03, 0x0a,
    0x11, 0x18, 0x20, 0x19, 0x12, 0x0b, 0x04, 0x05,
    0x0c, 0x13, 0x1a, 0x21, 0x28, 0x30, 0x29, 0x22,
    0x1b, 0x14, 0x0d, 0x06, 0x07, 0x0e, 0x15, 0x1c,
    0x23, 0x2a, 0x31, 0x38, 0x39, 0x32, 0x2b, 0x24,
    0x1d, 0x16, 0x0f, 0x17, 0x1e, 0x25, 0x2c, 0x33,
    0x3a, 0x3b, 0x34, 0x2d, 0x26, 0x1f, 0x27, 0x2e,
    0x35, 0x3c, 0x3d, 0x36, 0x2f, 0x37, 0x3e, 0x3f
};

const u_int32 vlbInvZigzagScan[64] = 
{
    0x00, 0x01, 0x05, 0x06, 0x0e, 0x0f, 0x1b, 0x1c,
    0x1c, 0x1c, 0x1c, 0x1c, 0x1c, 0x1c, 0x1d, 0x2a,
    0x2a, 0x2a, 0x2a, 0x2a, 0x2a, 0x2a, 0x2a, 0x2b,
    0x2b, 0x2b, 0x2b, 0x2b, 0x2b, 0x2b, 0x2c, 0x35,
    0x35, 0x35, 0x35, 0x35, 0x35, 0x35, 0x35, 0x36,
    0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x37, 0x3c,
    0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x3d,
    0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x3e, 0x3f
};


const tVLCTable vlbLumaTableIntraDC[13] = 
{
    {0x03,0x03}, {0x03,0x02}, {0x02,0x02}, {0x02,0x03}, 
        {0x01,0x03}, {0x01,0x04}, {0x01,0x05}, {0x01,0x06}, 
        {0x01,0x07}, {0x01,0x08}, {0x01,0x09}, {0x01,0x0a}, 
        {0x01,0x0b}
};

const tVLCTable vlbChromaTableIntraDC[13] =
{
    {0x03,0x02}, {0x02,0x02}, {0x01,0x02}, {0x01,0x03}, 
        {0x01,0x04}, {0x01,0x05}, {0x01,0x06}, {0x01,0x07}, 
        {0x01,0x08}, {0x01,0x09}, {0x01,0x0a}, {0x01,0x0b}
};

/* Array of pointers for AC VLC (INTER)tables */
const tVLCTable * const vlbVLCTablePointer0[27] =
{
    vlbVLCTable1InterAC,
    vlbVLCTable2InterAC,
    vlbVLCTable3InterAC,
    vlbVLCTable4InterAC[0],
    vlbVLCTable4InterAC[1],
    vlbVLCTable4InterAC[2],
    vlbVLCTable4InterAC[3],
    vlbVLCTable5InterAC[0],
    vlbVLCTable5InterAC[1],
    vlbVLCTable5InterAC[2],
    vlbVLCTable5InterAC[3],
    vlbVLCTable6InterAC - 0x01, // this pointer is going outside the range, but it is used only with condition (level < vlbVLCRun0InterAC[run]) which results having indexes > 0 and hence the array is never indexed below its base address
    vlbVLCTable6InterAC ,
    vlbVLCTable6InterAC + 0x01,
    vlbVLCTable6InterAC + 0x02,
    vlbVLCTable6InterAC + 0x03,
    vlbVLCTable6InterAC + 0x04,
    vlbVLCTable6InterAC + 0x05,
    vlbVLCTable6InterAC + 0x06,
    vlbVLCTable6InterAC + 0x07,
    vlbVLCTable6InterAC + 0x08,
    vlbVLCTable6InterAC + 0x09,
    vlbVLCTable6InterAC + 0x0a,
    vlbVLCTable6InterAC + 0x0b,
    vlbVLCTable6InterAC + 0x0c,
    vlbVLCTable6InterAC + 0x0d,
    vlbVLCTable6InterAC + 0x0e

};

const tVLCTable * const vlbVLCTablePointer1[41] =
{
    vlbVLCTable7InterAc,
    vlbVLCTable8InterAC,
    vlbVLCTable9InterAC - 0x01, // this pointer is going outside the range, but it is used only with condition (level < vlbVLCRun1InterAC[run]) which results having indexes > 0 and hence the array is never indexed below its base address
    vlbVLCTable9InterAC ,
    vlbVLCTable9InterAC + 0x01,
    vlbVLCTable9InterAC + 0x02,
    vlbVLCTable9InterAC + 0x03,
    vlbVLCTable9InterAC + 0x04,
    vlbVLCTable9InterAC + 0x05,
    vlbVLCTable9InterAC + 0x06,
    vlbVLCTable9InterAC + 0x07,
    vlbVLCTable9InterAC + 0x08,
    vlbVLCTable9InterAC + 0x09,
    vlbVLCTable9InterAC + 0x0a,
    vlbVLCTable9InterAC + 0x0b,
    vlbVLCTable9InterAC + 0x0c,
    vlbVLCTable9InterAC + 0x0d,
    vlbVLCTable9InterAC + 0x0e,
    vlbVLCTable9InterAC + 0x0f,
    vlbVLCTable9InterAC + 0x10,
    vlbVLCTable9InterAC + 0x11,
    vlbVLCTable9InterAC + 0x12,
    vlbVLCTable9InterAC + 0x13,
    vlbVLCTable9InterAC + 0x14,
    vlbVLCTable9InterAC + 0x15,
    vlbVLCTable9InterAC + 0x16,
    vlbVLCTable9InterAC + 0x17,
    vlbVLCTable9InterAC + 0x18,
    vlbVLCTable9InterAC + 0x19,
    vlbVLCTable9InterAC + 0x1a,
    vlbVLCTable9InterAC + 0x1b,
    vlbVLCTable9InterAC + 0x1c,
    vlbVLCTable9InterAC + 0x1d,
    vlbVLCTable9InterAC + 0x1e,
    vlbVLCTable9InterAC + 0x1f,
    vlbVLCTable9InterAC + 0x20,
    vlbVLCTable9InterAC + 0x21,
    vlbVLCTable9InterAC + 0x22,
    vlbVLCTable9InterAC + 0x23,
    vlbVLCTable9InterAC + 0x24,
    vlbVLCTable9InterAC + 0x25,
};

const int8 vlbVLCRun0IntraAC[64] =
{
    0x1c, 0x0b, 0x06, 0x05, 0x04, 0x04, 0x04, 0x04, 
      0x03, 0x03, 0x02, 0x02, 0x02, 0x02, 0x02, 0x00, 
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 
};

const int8 vlbVLCRun1IntraAC[64] =
{
    0x09, 0x04, 0x03, 0x03, 0x03, 0x03, 0x03, 0x02, 
    0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 
      0x02, 0x02, 0x02, 0x02, 0x02, 0x00, 0x00, 0x00, 
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 
};

/* array of table pointers for AC VLC INTRA tables*/

const tVLCTable * const vlbVLCTableIntraPointer0[15] =
{
    vlbVLCTable1IntraAC,
    vlbVLCTable2IntraAC,
    vlbVLCTable3IntraAC,
    vlbVLCTable4IntraAC,
    vlbVLCTable5IntraAC[0],
    vlbVLCTable5IntraAC[1],
    vlbVLCTable5IntraAC[2],
    vlbVLCTable5IntraAC[3],
    vlbVLCTable6IntraAC[0],
    vlbVLCTable6IntraAC[1],
    vlbVLCTable7IntraAC[0],
    vlbVLCTable7IntraAC[1],
    vlbVLCTable7IntraAC[2],
    vlbVLCTable7IntraAC[3],
    vlbVLCTable7IntraAC[4],
};

const tVLCTable * const vlbVLCTableIntraPointer1[21] =
{
    vlbVLCTable8IntraAC,
    vlbVLCTable9IntraAC,
    vlbVLCTable10IntraAC[0],
    vlbVLCTable10IntraAC[1],
    vlbVLCTable10IntraAC[2],
    vlbVLCTable10IntraAC[3],
    vlbVLCTable10IntraAC[4],
    vlbVLCTable11IntraAC[0],
    vlbVLCTable11IntraAC[1],
    vlbVLCTable11IntraAC[2],
    vlbVLCTable11IntraAC[3],
    vlbVLCTable11IntraAC[4],
    vlbVLCTable11IntraAC[5],
    vlbVLCTable11IntraAC[6],
    vlbVLCTable11IntraAC[7],
    vlbVLCTable11IntraAC[8],
    vlbVLCTable11IntraAC[9],
    vlbVLCTable11IntraAC[10],
    vlbVLCTable11IntraAC[11],
    vlbVLCTable11IntraAC[12],
    vlbVLCTable11IntraAC[13],
};


/* {{-output"vdtPutIntraMBCMT.txt"}} */
/*
* vdtPutIntraMBCMT
*
* Parameters:
*     outBuf                output buffer
*       coeff           pointer to the coefficients that must be in scanned order
*       numTextureBits  pointer to store the number of bits taken to encode the macro block.
*       index           block Index
*       skipDC          whether to skip INTRA DC
*       skipAC          whether to skip INTRA AC
*
* Function:
*     This function forms the bitstream for INTRA macroblock in combined motion and 
*     texture mode for the transcoding module
*
* Returns:
*     Nothing.
*
* Error codes:
*     None.
*
*/
void vdtPutIntraMBCMT(bibBuffer_t *outBuf, int *coeff, int *numTextureBits, int index, int skipDC, int skipAC)
{
    int32   textureBits;
    textureBits = 0;
    
    // DC coefficient VLC
    if (!skipDC)
    {
       textureBits += vlbPutDCCoeffCMT(coeff[0], outBuf, index);
    }
    
    //AC coefficient VLC
    if (!skipAC)
    {
        textureBits += vlbPutIntraACCoeffCMT(1, coeff, outBuf);
        *numTextureBits = textureBits;
    }
    
    return;
}


/* {{-output"vdtPutInterMBCMT.txt"}} */
/*
* vdtPutInterMBCMT
*
* Parameters:
*     outBuf                output buffer
*       coeff           pointer to the coefficients that must be in scanned order
*       numTextureBits  pointer to store the number of bits taken to encode the macro block.
*       svh             flag to indicate short video header
*
* Function:
*     This function forms the bitstream for INTER macroblock in combined motion and 
*     texture mode for the transcoding module
*
* Returns:
*     Nothing.
*
* Error codes:
*     None.
*
*/
void vdtPutInterMBCMT(bibBuffer_t *outBuf, int coeffStart, int *coeff, int *numTextureBits, int svh)
{
    int32   textureBits;
    int16 coeffInt16[64];

    /* encode texture coefficients */
    textureBits = 0;
    /* find out the last nonzero coeficients */
    int nonZeroDCT = 0;
    for (int i = 0; i < 64; i++)
    {
        nonZeroDCT = coeff[i] != 0 ? i : nonZeroDCT;
        coeffInt16[i] = (int16)coeff[i];
    }
    textureBits += vlbPutACCoeffSVH(coeffStart, coeffInt16, outBuf, svh, nonZeroDCT);
    *numTextureBits = textureBits;
    
    return;
}



/* {{-output"vlbPutDCCoeffCMT.txt"}} */
/*
* vlbPutDCCoeffCMT
*
* Parameters:
*     outBuf                output buffer
*       val             value of the DC coefficient
*       blkCnt          block number in the macro block
*
* Function:
*     This function puts DC coefficient in CMT mode into the bitstream
*
* Returns:
*     number of bits outputted.
*
* Error codes:
*     None.
*
*/
int32 vlbPutDCCoeffCMT(int val, bibBuffer_t * outBuf, int32  blkCnt)
{
    int32   absVal;
    int32   numBits;
    int32   size;
    
    absVal = ABS(val);
    size = 0;
    
    while(absVal > 0)
    {
        size++;
        absVal >>= 1;
    }
    
    numBits = size;
    if(blkCnt < 4)
    {
        vlbPutBits(outBuf, vlbLumaTableIntraDC[size].length, vlbLumaTableIntraDC[size].code); 
        numBits += vlbLumaTableIntraDC[size].length;
    }
    else
    {
        vlbPutBits(outBuf, vlbChromaTableIntraDC[size].length, vlbChromaTableIntraDC[size].code); 
        numBits += vlbChromaTableIntraDC[size].length;
    }
    
    if(size)
    {
        if(val > 0)
        {
            vlbPutBits(outBuf, size, val); 
        }
        else
        {
            absVal = ((1 << size) - 1);
            vlbPutBits(outBuf, size, (absVal + val)); 
        }
    }
    
    if(size > 8)
    {
        vlbPutBits(outBuf, 1, 1);
        numBits++;
    }
    
    return numBits;
}



/* {{-output"vlbPutIntraACCoeffCMT.txt"}} */
/*
* vlbPutIntraACCoeffCMT
*
* Parameters:
*     outBuf                output buffer
*       coeffStart      coefficient number from where the coding is to start
*       block           pointer to the texture block data
*
* Function:
*     This function puts AC coefficients in CMT mode into the bitstream
*
* Returns:
*     number of bits outputted.
*
* Error codes:
*     None.
*
*/
int32 vlbPutIntraACCoeffCMT(int32 coeffStart, int* block, bibBuffer_t * outBuf) 
{
    tEvent      prevEvent;
    tEvent      currEvent;
    int32       i;
    int32       numBits;
    tEvent      *prev = &prevEvent;
    tEvent      *curr = &currEvent;
    int     level;
    
    curr->run  = 0;
    prev->last = 0;
    numBits    = 0;
    
    for(i = coeffStart; i < 64; i++)
    {
        level = block[i]; 
        if(level == 0)
        {
            curr->run++;
        }
        else
        {
            prev->level = level;
            prev->run = curr->run;
            curr->run  = 0;
            break;
        }
    }
    
    for(i = i+1; i < 64; i++)
    {
        level = block[i]; 
        if(level == 0)
        {
            curr->run++;
        }
        else
        {
            numBits  += vlbPutIntraACCoeffCMTDP(outBuf, &prevEvent);
            prev->level = level;
            prev->run = curr->run;
            curr->run = 0;
        }
    }
    
    prev->last = 1;
    numBits  += vlbPutIntraACCoeffCMTDP(outBuf, &prevEvent);
    
    return numBits;
}



/* {{-output"vlbFindRMax.txt"}} */
/*
* vlbFindRMax
*
* Parameters:
*     event           pointer to the event being coded
*       mbType          macro block encoding type (INTER/INTRA)
*
* Function:
*     This function finds RMAX value to subtract from run for escape coding 
*     when reversible VLC is OFF
*
* Returns:
*     RMAX value
*
* Error codes:
*     None.
*
*/
int32 vlbFindRMax(tEvent *event, int16 mbType)
{
    int32 level;
    
    level = ABS(event->level);
    
    if(mbType == INTRA)
    {
        if(event->last == 0)
        {
            return level <= 27 ? vlbRMaxTableIntra[level - 1] : 
            NOT_VALID;
        }
        else
        {
            return level <= 8 ? vlbRMaxTableIntra[level + 26] :
            NOT_VALID;
        }
    }
    else
    {
        if (event->last == 0)
        {
            return level <= 12 ? vlbRMaxTableInter[level - 1] : 
            NOT_VALID;
        }
        else
        {
            return level <= 3 ? vlbRMaxTableInter[level + 11]
                : NOT_VALID; 
        }
    }
}



/* {{-output"vlbFindLMax.txt"}} */
/*
* vlbFindLMax
*
* Parameters:
*     event           pointer to the event being coded
*       mbType          macro block encoding type (INTER/INTRA)
*
* Function:
*     This function finds LMAX value to subtract from run for escape coding 
*     when reversible VLC is OFF
*
* Returns:
*     LMAX value
*
* Error codes:
*     None.
*
*/
int32 vlbFindLMax(tEvent *event, int16 mbType)
{
    if (mbType == INTRA)
    {
        if (event->last == 0)
        {
            if (event->run <= 14)
            {
                return vlbLMaxTableIntra[event->run];
            }
            else
            {
                return NOT_VALID;
            }
        }
        else
        {
            if (event->run <= 20)
            {
                return vlbLMaxTableIntra[event->run + 15];
            }
            else
            {
                return NOT_VALID;
            }
        }
    }
    else
    {
        if (event->last == 0)
        {
            return event->run <= 26 ? vlbLMaxTableInter[event->run] : 
            NOT_VALID;
        }
        else
        {
            return event->run <= 40 ? vlbLMaxTableInter[event->run + 27] :
            NOT_VALID;
        }
    }
}



/* {{-output"vlbPutIntraACCoeffCMTDP.txt"}} */
/*
* vlbPutIntraACCoeffCMTDP
*
* Parameters:
*     outBuf                output buffer
*       event           pointer to the event being coded
*
* Function:
*     This function encodes AC coefficient for Intra blocks in combined motion texture 
                    mode and data partitioned mode
*
* Returns:
*     number of bits outputted.
*
* Error codes:
*     None.
*
*/
int32 vlbPutIntraACCoeffCMTDP(bibBuffer_t *outBuf, tEvent *event)
{
    int32       sign;
    int32       level;
    int32       run;
    int32       status;
    int32       count;
    const tVLCTable   *vlcTable=NULL;
    int32       lmax;
    int32       rmax;
    
    sign  = (event->level >> 15) & (0x1);
    level =  ABS(event->level);
    run   = event->run;
    count = 0;
    
    do
    {
        status = CODE_FOUND;
        if(event->last == 0)
        {
            if(level < vlbVLCRun0IntraAC[run] )
            {
                vlcTable = vlbVLCTableIntraPointer0[run];
            }           
            else 
            {
                status = CODE_NOT_FOUND;
            }
        }
        else
        {
            if(level < vlbVLCRun1IntraAC[run] )
            {
                vlcTable = vlbVLCTableIntraPointer1[run];
            }           
            else 
            {
                status = CODE_NOT_FOUND;
            }
        }
        
        if (status == CODE_NOT_FOUND)
        {
            switch (++count)
            {
            case 1:
                {
                    lmax = vlbFindLMax(event, INTRA);
                    if (lmax == NOT_VALID)
                    {
                        count = 2;
                    }
                    else
                    {
                        level -= lmax;
                        break;
                    }
                }
            case 2:
                {
                    rmax = vlbFindRMax(event, INTRA);
                    if (rmax != NOT_VALID)
                    {
                        level = ABS(event->level);
                        run   = event->run - rmax;
                        break;
                    }
                    else
                    {
                        count = 3;
                    }
                }
            case 3:
                {
                    status = CODE_FOUND;
                }
            }
        }
        
    } while (status != CODE_FOUND);
    
    switch (count)
    {
    case 0:
        vlbPutBits(outBuf, vlcTable[level].length, vlcTable[level].code | sign);
        return vlcTable[level].length;
        
    case 1:
        vlbPutBits(outBuf, ESCAPE_CODE_LENGTH_VLC, ESCAPE_CODE_VLC);
        vlbPutBits(outBuf, 1, 0);
        vlbPutBits(outBuf, vlcTable[level].length, vlcTable[level].code | sign);
        return vlcTable[level].length + 1 + ESCAPE_CODE_LENGTH_VLC;
        
    case 2:
        vlbPutBits(outBuf, ESCAPE_CODE_LENGTH_VLC, ESCAPE_CODE_VLC);
        vlbPutBits(outBuf, 2, 0x2);
        vlbPutBits(outBuf, vlcTable[level].length, vlcTable[level].code | sign);
        return vlcTable[level].length + 2 + ESCAPE_CODE_LENGTH_VLC;
        
    case 3:
        vlbPutBits(outBuf, ESCAPE_CODE_LENGTH_VLC, ESCAPE_CODE_VLC);
        vlbPutBits(outBuf, 2, 0x3);
        vlbPutBits(outBuf, 8, (event->last << 7) | (event->run << 1) | 1);
        vlbPutBits(outBuf, 13, ((event->level & 0x0fff) << 1) | 1);
        return 23 + ESCAPE_CODE_LENGTH_VLC;
        
    default:
        return E_FAILURE;
    }
}



/* {{-output"vlbPutACCoeffSVH.txt"}} */
/*
* vlbPutACCoeffSVH
*
* Parameters:
*     outBuf                output buffer
*       coeffStart      coefficient number from where the coding to be started
*       block           pointer to the texture block data. Coeficients must be in zigzag order
*       svh             flag indicating whether short video header mode is ON
*       lastPos         value of last non-zero position of block
*
* Function:
*     This function puts AC coefficients in SVH mode into the bitstream
*
* Returns:
*     number of bits outputted.
*
* Error codes:
*     None.
*
*/
int32 vlbPutACCoeffSVH(int32 coeffStart, int16* block, bibBuffer_t * outBuf,
                       int32 svh, int32 lastPos)
                                             
{
    tEvent  prevEvent;
    tEvent  currEvent;
    int32   i;
    int32   numBits;
    tEvent  *prev ;
    tEvent  *curr ;
    int32       level;
    int32   lastPosition = lastPos;
    
    prev = &prevEvent;
    curr = &currEvent;
    curr->run  = 0;
    prev->last = 0;
    numBits = 0;
    
    for(i = coeffStart; i <= lastPosition; i++)
    {
        level = (int16)block[i];
        if(level == 0)
        {
            curr->run++;
        }
        else
        {
            // clip the coeff (MPEG-4 has larger range than H.263) requantization of the whole block is too complicated for this minor use case
            if ( level < -127 )
                {
                level = -127;
                }
            else if ( level > 127 )
                {
                level = 127;
                }
            prev->level = level;
            prev->run = curr->run;
            curr->run  = 0;
            break;
        }
    }
    
    for(i = i+1; i <= lastPosition; i++)
    {
        level = (int16)block[i];
        if(level == 0)
        {
            curr->run++;
        }
        else
        {
            numBits  += vlbPutACVLCCoeff(outBuf, &prevEvent,svh);
            
            // clip the coeff (MPEG-4 has larger range than H.263) requantization of the whole block is too complicated for this minor use case
            if ( level < -127 )
                {
                level = -127;
                }
            else if ( level > 127 )
                {
                level = 127;
                }
            prev->level = level;
            prev->run = curr->run;
            curr->run = 0;
        }
    }
    
    prev->last = 1;
    numBits  += vlbPutACVLCCoeff(outBuf, &prevEvent,svh);
    
    return numBits;
}



/* {{-output"vlbCodeACCoeffsSVHWithZigZag.txt"}} */
/*
* vlbCodeACCoeffsSVHWithZigZag
*
* Parameters:
*     outBuf                output buffer
*       coeffStart      coefficient number from where the coding to be started
*       block           pointer to the texture block data. Coeficients must be in zigzag order
*       svh             flag indicating whether short video header mode is ON
*       lastPos         value of last non-zero position of block
*
* Function:
*     This function puts AC coefficients in SVH mode with zigzag into the bitstream
*
* Returns:
*     number of bits outputted.
*
* Error codes:
*     None.
*
*/
int32 vlbCodeACCoeffsSVHWithZigZag(int32 coeffStart, int16* block, bibBuffer_t * outBuf,
                                                                 int32 svh, int32 lastPos)
                                                                 
{
    tEvent      prevEvent;
    tEvent      currEvent;
    int32       i;
    int32       numBits;
    tEvent      *prev ;
    tEvent      *curr ;
    int32       level;
    int32       lastPosition = vlbInvZigzagScan[lastPos];
    
    prev = &prevEvent;
    curr = &currEvent;
    curr->run  = 0;
    prev->last = 0;
    numBits = 0;
    
    for(i = coeffStart; i <= lastPosition; i++)
    {
        level = block[vlbZigzagScan[i]]; 
        if(level == 0)
        {
            curr->run++;
        }
        else
        {
            prev->level = level;
            prev->run = curr->run;
            curr->run  = 0;
            break;
        }
    }
    
    for(i = i+1; i <= lastPosition; i++)
    {
        level = block[vlbZigzagScan[i]]; 
        if(level == 0)
        {
            curr->run++;
        }
        else
        {
            numBits  += vlbPutACVLCCoeff(outBuf, &prevEvent,svh);
            prev->level = level;
            prev->run = curr->run;
            curr->run = 0;
        }
    }
    
    prev->last = 1;
    numBits  += vlbPutACVLCCoeff(outBuf, &prevEvent,svh);
    
    return numBits;
}




/* {{-output"vlbPutACVLCCoeff.txt"}} */
/*
* vlbPutACVLCCoeff
*
* Parameters:
*     outBuf                output buffer
*       event           pointer to the event being coded
*       svh             flag indicating whether short video header mode is ON
*
* Function:
*     This function encodes AC coefficients for Inter and Intra blocks in SVH mode and 
*     inter blocks in non-SVH mode
*
* Returns:
*     number of bits outputted.
*
* Error codes:
*     None.
*
*/
int32 vlbPutACVLCCoeff(bibBuffer_t *outBuf, tEvent *event, int32 svh)
{
    int32       sign;
    int32       run;
    int32       level;
    int32       status;
    int32       count;
    const tVLCTable   *vlcTable= NULL;
    int32       lmax;
    int32       rmax;
    
    sign  = (event->level >> 15) & (0x1);
    level = ABS(event->level);
    run   = event->run;
    count = 0;
    
    do
    {
        status = CODE_FOUND;
        if(event->last == 0)
        {
            if(level < vlbVLCRun0InterAC[run] )
            {
                vlcTable = vlbVLCTablePointer0[run];
            }           
            else 
            {
                status = CODE_NOT_FOUND;
            }
        }
        else
        {
            if(level < vlbVLCRun1InterAC[run] )
            {
                vlcTable = vlbVLCTablePointer1[run];
            }           
            else 
            {
                status = CODE_NOT_FOUND;
            }
        }
        
        if (status == CODE_NOT_FOUND)
        {
            if (svh == ON)
            {
                count  = 4;
                status = CODE_FOUND;
            }
            else
            {
                switch (++count)
                {
                case 1:
                    {
                        lmax = vlbFindLMax(event, INTER);
                        if (lmax == NOT_VALID)
                        {
                            count = 2;
                        }
                        else
                        {
                            level = ABS(event->level) - lmax;
                            break;
                        }
                    }
                case 2:
                    {
                        rmax = vlbFindRMax(event, INTER);
                        if (rmax != NOT_VALID)
                        {
                            level = ABS(event->level);
                            run   = event->run - rmax;
                            break;
                        }
                        else
                        {
                            count = 3;
                        }
                    }
                case 3:
                    {
                        status = CODE_FOUND;
                        break;
                    }
                default:
                    {
                        return E_FAILURE;
                    }
                }
            }
        }
        
    } while (status != CODE_FOUND);
    
    switch (count)
    {
    case 0:
        {
            vlbPutBits(outBuf, vlcTable[level].length, vlcTable[level].code | sign);
            return vlcTable[level].length;
        }
    case 1:
        {
            vlbPutBits(outBuf, ESCAPE_CODE_LENGTH_VLC, ESCAPE_CODE_VLC);
            vlbPutBits(outBuf, 1, 0);
            vlbPutBits(outBuf, vlcTable[level].length, vlcTable[level].code | sign);
            return vlcTable[level].length + 1 + ESCAPE_CODE_LENGTH_VLC;
        }
    case 2:
        {
            vlbPutBits(outBuf, ESCAPE_CODE_LENGTH_VLC, ESCAPE_CODE_VLC);
            vlbPutBits(outBuf, 2, 0x2);
            vlbPutBits(outBuf, vlcTable[level].length, vlcTable[level].code | sign);
            return vlcTable[level].length + 2 + ESCAPE_CODE_LENGTH_VLC;
        }
    case 3:
        {
            vlbPutBits(outBuf, ESCAPE_CODE_LENGTH_VLC, ESCAPE_CODE_VLC);
            vlbPutBits(outBuf, 2, 0x3);
            /* Fixed Length Coding of Events */
            vlbPutBits(outBuf, 8, (event->last << 7) | (event->run << 1) | 1);
            vlbPutBits(outBuf, 13, ((event->level & 0x0fff) << 1) | 1);
            return 23 + ESCAPE_CODE_LENGTH_VLC;
        }
    case 4:
        {
            vlbPutBits(outBuf, ESCAPE_CODE_LENGTH_VLC, ESCAPE_CODE_VLC);
            vlbPutBits(outBuf, 7, (event->last << 6) | event->run);
            vlbPutBits(outBuf, 8, event->level & 0x0ff);
            return 15 + ESCAPE_CODE_LENGTH_VLC;
        }
    default:
        {
            return E_FAILURE;
        }
    }
}

/* End of vlb.cpp */


